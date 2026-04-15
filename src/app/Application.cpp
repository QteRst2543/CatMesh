#include "app/Application.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "core/Mesh.h"
#include "core/Grid.h"
#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef APIENTRY
#undef APIENTRY
#endif
#include <windows.h>
#include <commdlg.h>
#endif

namespace {

static void scroll_callback(GLFWwindow* window, double, double yoffset) {
    void* userPtr = glfwGetWindowUserPointer(window);
    if (userPtr) {
        Application* app = static_cast<Application*>(userPtr);
        app->GetCamera().Zoom(static_cast<float>(yoffset));
    }
}

bool IsKeyPressedOnce(GLFWwindow* window, int key) {
    static std::unordered_map<int, bool> wasPressed;
    const bool pressed = glfwGetKey(window, key) == GLFW_PRESS;
    const bool result = pressed && !wasPressed[key];
    wasPressed[key] = pressed;
    return result;
}

#ifdef _WIN32
std::string ShowWindowsFileDialog(const char* filter, DWORD flags, const char* defaultExtension) {
    char fileName[MAX_PATH] = {};

    OPENFILENAMEA dialogConfig = {};
    dialogConfig.lStructSize = sizeof(dialogConfig);
    dialogConfig.hwndOwner = nullptr;
    dialogConfig.lpstrFile = fileName;
    dialogConfig.nMaxFile = MAX_PATH;
    dialogConfig.lpstrFilter = filter;
    dialogConfig.nFilterIndex = 1;
    dialogConfig.Flags = flags;
    dialogConfig.lpstrDefExt = defaultExtension;

    const BOOL result = (flags & OFN_FILEMUSTEXIST)
        ? GetOpenFileNameA(&dialogConfig)
        : GetSaveFileNameA(&dialogConfig);

    return result ? std::string(fileName) : std::string();
}
#endif

std::string ToLowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

} // namespace

Application::~Application() = default;

std::shared_ptr<Mesh> Application::GetSelectedMeshHandle() const {
    auto it = std::find_if(meshes.begin(), meshes.end(),
        [this](const auto& mesh) { return mesh.get() == selectedMesh; });
    return it != meshes.end() ? *it : nullptr;
}

glm::vec3 Application::GetSceneCenter() const {
    if (meshes.empty()) {
        return glm::vec3(0.0f);
    }

    glm::vec3 center(0.0f);
    for (const auto& mesh : meshes) {
        center += mesh->GetPosition();
    }
    return center / static_cast<float>(meshes.size());
}

glm::vec3 Application::GetOrbitFocus() const {
    return selectedMesh ? selectedMesh->GetPosition() : GetSceneCenter();
}

Application::Application()
    : window(1920, 1080, "CatMesh"),
      ui(window, this),
      lastLeftMouseX(0.0f),
      lastLeftMouseY(0.0f),
      lastRightMouseX(0.0f),
      lastRightMouseY(0.0f),
      leftMousePressed(false),
      rightMousePressed(false),
      dragStartMouseX(0.0),
      dragStartMouseY(0.0),
      isDraggingElement(false),
      objectDragOffset(0.0f),
      hasObjectDragOffset(false),
      lastDragIntersection(0.0f),
      hasLastDragIntersection(false),
      dragPlanePoint(0.0f),
      dragPlaneNormal(0.0f, 1.0f, 0.0f),
      hasDragPlane(false) {
    if (!window.IsValid()) {
        std::cerr << "Application initialization aborted: window/context creation failed." << std::endl;
        return;
    }

    camera.Update();
    AddDefaultCube();

    GLFWwindow* nativeWindow = window.GetNativeWindow();
    glfwSetWindowUserPointer(nativeWindow, this);
    glfwSetScrollCallback(nativeWindow, scroll_callback);
}

void Application::ClearHistory() {
    while (!undoStack.empty()) {
        undoStack.pop();
    }
    while (!redoStack.empty()) {
        redoStack.pop();
    }
}

void Application::AddDefaultCube() {
    auto mesh = std::make_shared<Mesh>();
    if (!mesh->LoadFromFile("")) {
        std::cerr << "Failed to create default cube." << std::endl;
        return;
    }

    mesh->SetPosition(glm::vec3(0.0f));
    mesh->SetColor(glm::vec3(0.5f, 0.5f, 0.5f));
    mesh->SetEditMode(Mesh::EditMode::Object);
    selectedMesh = mesh.get();
    meshes.push_back(mesh);
}

void Application::DeleteSelectedMesh() {
    if (!selectedMesh) {
        return;
    }

    auto it = std::find_if(meshes.begin(), meshes.end(),
        [this](const auto& ptr) { return ptr.get() == selectedMesh; });
    if (it == meshes.end()) {
        return;
    }

    ClearHistory();
    meshes.erase(it);
    selectedMesh = meshes.empty() ? nullptr : meshes.front().get();
}

void Application::ExecuteCommand(std::unique_ptr<Command> cmd) {
    cmd->Execute();
    undoStack.push(std::move(cmd));
    while (!redoStack.empty()) {
        redoStack.pop();
    }
}

void Application::Undo() {
    if (undoStack.empty()) {
        return;
    }

    auto cmd = std::move(undoStack.top());
    undoStack.pop();
    cmd->Undo();
    redoStack.push(std::move(cmd));
}

void Application::Redo() {
    if (redoStack.empty()) {
        return;
    }

    auto cmd = std::move(redoStack.top());
    redoStack.pop();
    cmd->Execute();
    undoStack.push(std::move(cmd));
}

Mesh* Application::AddMesh(const std::shared_ptr<Mesh>& mesh) {
    Mesh* raw = mesh.get();
    selectedMesh = raw;
    meshes.push_back(mesh);
    return raw;
}

void Application::RemoveMesh(Mesh* mesh) {
    auto it = std::find_if(meshes.begin(), meshes.end(),
        [mesh](const auto& ptr) { return ptr.get() == mesh; });
    if (it == meshes.end()) {
        return;
    }

    meshes.erase(it);
    if (selectedMesh == mesh) {
        selectedMesh = meshes.empty() ? nullptr : meshes.front().get();
    }
}

glm::vec3 Application::GetRayFromMouse(double mouseX, double mouseY, int screenWidth, int screenHeight) {
    const float ndcX = (2.0f * static_cast<float>(mouseX)) / static_cast<float>(screenWidth) - 1.0f;
    const float ndcY = 1.0f - (2.0f * static_cast<float>(mouseY)) / static_cast<float>(screenHeight);

    const glm::mat4 projection = camera.GetProjectionMatrix(static_cast<float>(screenWidth) / static_cast<float>(screenHeight));
    const glm::mat4 view = camera.GetViewMatrix();

    const glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    return glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
}

glm::vec3 Application::GetPlaneIntersection(
    const glm::vec3& rayOrigin,
    const glm::vec3& rayDir,
    const glm::vec3& planePoint,
    const glm::vec3& planeNormal
) {
    const float denominator = glm::dot(planeNormal, rayDir);
    if (std::fabs(denominator) < 0.0001f) {
        return planePoint;
    }

    const float t = glm::dot(planePoint - rayOrigin, planeNormal) / denominator;
    if (t < 0.0f) {
        return planePoint;
    }

    return rayOrigin + rayDir * t;
}

void Application::HandleCameraKeyboard(GLFWwindow* glfwWindow, float deltaTime, bool wantsKeyboard) {
    if (wantsKeyboard || !glfwWindow) {
        return;
    }

    float forwardAmount = 0.0f;
    float rightAmount = 0.0f;
    float moveSpeed = 4.0f * deltaTime;
    if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(glfwWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
        moveSpeed *= 2.0f;
    }

    if (glfwGetKey(glfwWindow, GLFW_KEY_W) == GLFW_PRESS) {
        forwardAmount += moveSpeed;
    }
    if (glfwGetKey(glfwWindow, GLFW_KEY_S) == GLFW_PRESS) {
        forwardAmount -= moveSpeed;
    }
    if (glfwGetKey(glfwWindow, GLFW_KEY_D) == GLFW_PRESS) {
        rightAmount += moveSpeed;
    }
    if (glfwGetKey(glfwWindow, GLFW_KEY_A) == GLFW_PRESS) {
        rightAmount -= moveSpeed;
    }

    if (forwardAmount != 0.0f || rightAmount != 0.0f) {
        camera.MoveTargetLocal(forwardAmount, rightAmount);
    }
}

void Application::HandleInput() {
    GLFWwindow* glfwWindow = window.GetNativeWindow();
    if (!glfwWindow) {
        return;
    }

    int screenWidth = 0;
    int screenHeight = 0;
    glfwGetFramebufferSize(glfwWindow, &screenWidth, &screenHeight);

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(glfwWindow, &mouseX, &mouseY);

    camera.SetMousePosition(static_cast<float>(mouseX), static_cast<float>(mouseY), screenWidth, screenHeight);

    const ImGuiIO& io = ImGui::GetIO();
    const bool wantsMouse = io.WantCaptureMouse;
    const bool wantsKeyboard = io.WantCaptureKeyboard;

    const bool leftDown = glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool rightDown = glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

    const glm::vec3 cameraPos = camera.GetPosition();
    const glm::vec3 rayDir = GetRayFromMouse(mouseX, mouseY, screenWidth, screenHeight);

    auto findClosestMeshUnderCursor = [&]() -> Mesh* {
        Mesh* closestMesh = nullptr;
        float closestDistance = std::numeric_limits<float>::max();
        for (const auto& mesh : meshes) {
            float hitDistance = 0.0f;
            if (mesh->IntersectRay(cameraPos, rayDir, &hitDistance) && hitDistance < closestDistance) {
                closestDistance = hitDistance;
                closestMesh = mesh.get();
            }
        }
        return closestMesh;
    };

    if (wantsMouse) {
        if (!leftDown) {
            leftMousePressed = false;
            isDraggingElement = false;
            hasObjectDragOffset = false;
            hasLastDragIntersection = false;
            hasDragPlane = false;
        }

        if (!rightDown) {
            rightMousePressed = false;
        }
    }
    else {
        if (leftDown) {
            if (!leftMousePressed) {
                leftMousePressed = true;
                lastLeftMouseX = static_cast<float>(mouseX);
                lastLeftMouseY = static_cast<float>(mouseY);
                dragStartMouseX = mouseX;
                dragStartMouseY = mouseY;
                isDraggingElement = false;
                hasObjectDragOffset = false;
                hasLastDragIntersection = false;
                hasDragPlane = false;

                Mesh* clickedMesh = findClosestMeshUnderCursor();
                if (clickedMesh) {
                    selectedMesh = clickedMesh;
                }

                if (selectedMesh && selectedMesh->GetEditMode() == Mesh::EditMode::Face && selectedMesh == clickedMesh) {
                    selectedMesh->UpdateSelectionFromMouse(cameraPos, rayDir);

                    if (selectedMesh->GetSelectedFace() >= 0) {
                        dragPlanePoint = selectedMesh->GetFaceCenter(selectedMesh->GetSelectedFace()) + selectedMesh->GetPosition();
                        dragPlaneNormal = camera.GetForward();
                        hasDragPlane = true;
                    }
                }
                else if (selectedMesh) {
                    dragPlanePoint = selectedMesh->GetPosition();
                    dragPlaneNormal = camera.GetForward();
                    hasDragPlane = true;

                    const glm::vec3 intersection = GetPlaneIntersection(cameraPos, rayDir, dragPlanePoint, dragPlaneNormal);
                    objectDragOffset = selectedMesh->GetPosition() - intersection;
                    hasObjectDragOffset = true;
                }
            }
            else {
                const float totalDeltaX = static_cast<float>(mouseX - dragStartMouseX);
                const float totalDeltaY = static_cast<float>(mouseY - dragStartMouseY);

                if (!isDraggingElement && (std::fabs(totalDeltaX) > 5.0f || std::fabs(totalDeltaY) > 5.0f)) {
                    isDraggingElement = true;
                }

                if (isDraggingElement && selectedMesh) {
                    if (selectedMesh->GetEditMode() == Mesh::EditMode::Object) {
                        if (!hasDragPlane) {
                            dragPlanePoint = selectedMesh->GetPosition();
                            dragPlaneNormal = camera.GetForward();
                            hasDragPlane = true;
                        }

                        glm::vec3 intersection = GetPlaneIntersection(cameraPos, rayDir, dragPlanePoint, dragPlaneNormal);

                        if (!hasObjectDragOffset) {
                            objectDragOffset = selectedMesh->GetPosition() - intersection;
                            hasObjectDragOffset = true;
                        }

                        glm::vec3 newPosition = intersection + objectDragOffset;
                        if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                            glfwGetKey(glfwWindow, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
                            newPosition.x = std::round(newPosition.x);
                            newPosition.y = std::round(newPosition.y);
                            newPosition.z = std::round(newPosition.z);
                        }

                        selectedMesh->SetPosition(newPosition);
                        dragPlanePoint = newPosition;
                    }
                    else if (selectedMesh->GetEditMode() == Mesh::EditMode::Face && selectedMesh->GetSelectedFace() >= 0) {
                        if (!hasDragPlane) {
                            dragPlanePoint = selectedMesh->GetFaceCenter(selectedMesh->GetSelectedFace()) + selectedMesh->GetPosition();
                            dragPlaneNormal = camera.GetForward();
                            hasDragPlane = true;
                        }

                        const glm::vec3 intersection = GetPlaneIntersection(cameraPos, rayDir, dragPlanePoint, dragPlaneNormal);
                        if (!hasLastDragIntersection) {
                            lastDragIntersection = intersection;
                            hasLastDragIntersection = true;
                        }
                        else {
                            const glm::vec3 delta = intersection - lastDragIntersection;
                            if (glm::length(delta) > 0.00001f) {
                                selectedMesh->DragSelectedElement(delta);
                                dragPlanePoint += delta;
                            }
                            lastDragIntersection = intersection;
                        }
                    }
                    else if (selectedMesh->GetEditMode() == Mesh::EditMode::Vertex && selectedMesh->GetSelectedVertex() >= 0) {
                        if (!hasDragPlane) {
                            dragPlanePoint = selectedMesh->GetVertexPosition(selectedMesh->GetSelectedVertex()) + selectedMesh->GetPosition();
                            dragPlaneNormal = camera.GetForward();
                            hasDragPlane = true;
                        }

                        const glm::vec3 intersection = GetPlaneIntersection(cameraPos, rayDir, dragPlanePoint, dragPlaneNormal);
                        if (!hasLastDragIntersection) {
                            lastDragIntersection = intersection;
                            hasLastDragIntersection = true;
                        }
                        else {
                            const glm::vec3 delta = intersection - lastDragIntersection;
                            if (glm::length(delta) > 0.00001f) {
                                selectedMesh->DragSelectedElement(delta);
                                dragPlanePoint += delta;
                            }
                            lastDragIntersection = intersection;
                        }
                    }
                }

                lastLeftMouseX = static_cast<float>(mouseX);
                lastLeftMouseY = static_cast<float>(mouseY);
            }
        }
        else {
            leftMousePressed = false;
            isDraggingElement = false;
            hasObjectDragOffset = false;
            hasLastDragIntersection = false;
            hasDragPlane = false;
        }

        if (rightDown) {
            if (!rightMousePressed) {
                rightMousePressed = true;
                lastRightMouseX = static_cast<float>(mouseX);
                lastRightMouseY = static_cast<float>(mouseY);
                camera.SetTarget(GetOrbitFocus());
            }
            else {
                const float deltaX = static_cast<float>(mouseX) - lastRightMouseX;
                const float deltaY = static_cast<float>(mouseY) - lastRightMouseY;
                camera.Rotate(deltaX, deltaY);
                lastRightMouseX = static_cast<float>(mouseX);
                lastRightMouseY = static_cast<float>(mouseY);
            }
        }
        else {
            rightMousePressed = false;
        }
    }

    if (!wantsKeyboard) {
        if (selectedMesh && IsKeyPressedOnce(glfwWindow, GLFW_KEY_1)) {
            selectedMesh->SetEditMode(Mesh::EditMode::Object);
        }
        if (selectedMesh && IsKeyPressedOnce(glfwWindow, GLFW_KEY_4)) {
            selectedMesh->SetEditMode(Mesh::EditMode::Face);
        }

        if (IsKeyPressedOnce(glfwWindow, GLFW_KEY_DELETE)) {
            DeleteSelectedMesh();
        }

        if (IsKeyPressedOnce(glfwWindow, GLFW_KEY_Y) &&
            (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
             glfwGetKey(glfwWindow, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)) {
            Redo();
        }

        if (IsKeyPressedOnce(glfwWindow, GLFW_KEY_Z) &&
            (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
             glfwGetKey(glfwWindow, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)) {
            Undo();
        }
    }
}

void Application::Run() {
    if (!window.IsValid()) {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    Grid grid;
    float lastFrameTime = static_cast<float>(glfwGetTime());

    while (!window.ShouldClose()) {
        const float currentTime = static_cast<float>(glfwGetTime());
        const float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        window.PollEvents();
        ui.NewFrame();
        HandleCameraKeyboard(window.GetNativeWindow(), deltaTime, ImGui::GetIO().WantCaptureKeyboard);
        HandleInput();
        ui.Render();

        int displayWidth = 0;
        int displayHeight = 0;
        glfwGetFramebufferSize(window.GetNativeWindow(), &displayWidth, &displayHeight);
        glViewport(0, 0, displayWidth, displayHeight);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect = static_cast<float>(displayWidth) / static_cast<float>(displayHeight);
        const glm::mat4 view = camera.GetViewMatrix();
        const glm::mat4 projection = camera.GetProjectionMatrix(aspect);

        grid.Draw(view, projection);

        for (auto& mesh : meshes) {
            mesh->Draw(
                view,
                projection,
                camera.GetPosition(),
                light.position,
                light.direction,
                light.color,
                light.intensity,
                light.ambientStrength,
                light.innerCutoffDegrees,
                light.outerCutoffDegrees);
        }

        if (selectedMesh) {
            glDisable(GL_DEPTH_TEST);
            grid.DrawAxes(view, projection, selectedMesh->GetPosition(), 0.75f);
            glEnable(GL_DEPTH_TEST);
        }

        ui.RenderDrawData();
        window.SwapBuffers();
    }
}

bool Application::LoadScene(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "Failed to open scene file: " << path << std::endl;
        return false;
    }

    std::string token;
    in >> token;
    if (token != "CATMESH_SCENE_V1") {
        std::cerr << "Unsupported scene format: " << path << std::endl;
        return false;
    }

    std::size_t meshCount = 0;
    in >> token >> meshCount;
    if (!in || token != "mesh_count") {
        std::cerr << "Scene header is malformed." << std::endl;
        return false;
    }

    std::vector<std::shared_ptr<Mesh>> loadedMeshes;
    loadedMeshes.reserve(meshCount);

    for (std::size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex) {
        in >> token;
        if (!in || token != "mesh") {
            std::cerr << "Expected mesh block in scene file." << std::endl;
            return false;
        }

        glm::vec3 position(0.0f);
        glm::vec3 color(0.5f, 0.5f, 0.5f);
        std::size_t vertexCount = 0;
        std::size_t faceCount = 0;
        std::vector<glm::vec3> editableVertices;
        std::vector<std::vector<int>> faces;

        in >> token >> position.x >> position.y >> position.z;
        if (!in || token != "position") {
            std::cerr << "Scene mesh position block is malformed." << std::endl;
            return false;
        }

        in >> token >> color.x >> color.y >> color.z;
        if (!in || token != "color") {
            std::cerr << "Scene mesh color block is malformed." << std::endl;
            return false;
        }

        in >> token >> vertexCount;
        if (!in || token != "vertex_count") {
            std::cerr << "Scene vertex block is malformed." << std::endl;
            return false;
        }

        editableVertices.reserve(vertexCount);
        for (std::size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
            glm::vec3 vertex(0.0f);
            in >> token >> vertex.x >> vertex.y >> vertex.z;
            if (!in || token != "v") {
                std::cerr << "Scene vertex entry is malformed." << std::endl;
                return false;
            }
            editableVertices.push_back(vertex);
        }

        in >> token >> faceCount;
        if (!in || token != "face_count") {
            std::cerr << "Scene face block is malformed." << std::endl;
            return false;
        }

        faces.reserve(faceCount);
        for (std::size_t faceIndex = 0; faceIndex < faceCount; ++faceIndex) {
            std::size_t indicesInFace = 0;
            in >> token >> indicesInFace;
            if (!in || token != "f" || indicesInFace < 3) {
                std::cerr << "Scene face entry is malformed." << std::endl;
                return false;
            }

            std::vector<int> face(indicesInFace, 0);
            for (std::size_t i = 0; i < indicesInFace; ++i) {
                in >> face[i];
            }

            if (!in) {
                std::cerr << "Failed to read face indices." << std::endl;
                return false;
            }

            faces.push_back(std::move(face));
        }

        in >> token;
        if (!in || token != "end_mesh") {
            std::cerr << "Scene mesh terminator is missing." << std::endl;
            return false;
        }

        auto mesh = std::make_shared<Mesh>();
        if (!mesh->SetGeometry(editableVertices, faces)) {
            return false;
        }

        mesh->SetPosition(position);
        mesh->SetColor(color);
        mesh->SetEditMode(Mesh::EditMode::Object);
        loadedMeshes.push_back(mesh);
    }

    meshes = std::move(loadedMeshes);
    selectedMesh = meshes.empty() ? nullptr : meshes.front().get();
    ClearHistory();
    return true;
}

bool Application::SaveSceneToPath(const std::string& path) const {
    std::ofstream out(path);
    if (!out.is_open()) {
        std::cerr << "Failed to open scene file for writing: " << path << std::endl;
        return false;
    }

    out << "CATMESH_SCENE_V1\n";
    out << "mesh_count " << meshes.size() << "\n";

    for (const auto& mesh : meshes) {
        out << "mesh\n";

        const glm::vec3 position = mesh->GetPosition();
        out << "position " << position.x << " " << position.y << " " << position.z << "\n";

        const glm::vec3 color = mesh->GetColor();
        out << "color " << color.x << " " << color.y << " " << color.z << "\n";

        const auto& editableVertices = mesh->GetEditableVertices();
        out << "vertex_count " << editableVertices.size() << "\n";
        for (const glm::vec3& vertex : editableVertices) {
            out << "v " << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
        }

        const auto& faces = mesh->GetFaces();
        out << "face_count " << faces.size() << "\n";
        for (const auto& face : faces) {
            out << "f " << face.size();
            for (int index : face) {
                out << " " << index;
            }
            out << "\n";
        }

        out << "end_mesh\n";
    }

    return true;
}

void Application::OpenFile() {
#ifdef _WIN32
    static const char filter[] =
        "CatMesh Scene (*.catmesh)\0*.catmesh\0"
        "Wavefront OBJ (*.obj)\0*.obj\0"
        "All Files (*.*)\0*.*\0\0";

    const std::string path = ShowWindowsFileDialog(
        filter,
        OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER,
        nullptr);
    if (path.empty()) {
        return;
    }

    const std::string extension = ToLowerCopy(std::filesystem::path(path).extension().string());
    if (extension == ".catmesh") {
        if (LoadScene(path)) {
            std::cout << "Loaded scene: " << path << std::endl;
        }
    }
    else {
        auto mesh = std::make_shared<Mesh>();
        if (!mesh->LoadFromFile(path)) {
            return;
        }

        mesh->SetPosition(glm::vec3(0.0f));
        mesh->SetColor(glm::vec3(0.7f, 0.7f, 0.7f));
        mesh->SetEditMode(Mesh::EditMode::Object);
        ClearHistory();
        selectedMesh = mesh.get();
        meshes.push_back(mesh);
        std::cout << "Imported mesh: " << path << std::endl;
    }
#else
    std::cerr << "OpenFile is only implemented with native dialogs on Windows." << std::endl;
#endif
}

void Application::SaveFile() {
#ifdef _WIN32
    static const char filter[] =
        "CatMesh Scene (*.catmesh)\0*.catmesh\0"
        "All Files (*.*)\0*.*\0\0";

    const std::string path = ShowWindowsFileDialog(
        filter,
        OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_EXPLORER,
        "catmesh");
    if (path.empty()) {
        return;
    }

    if (SaveSceneToPath(path)) {
        std::cout << "Saved scene: " << path << std::endl;
    }
#else
    std::cerr << "SaveFile is only implemented with native dialogs on Windows." << std::endl;
#endif
}

void Application::ExportSTL(const std::string& requestedPath) {
    if (meshes.empty()) {
        std::cout << "No meshes to export!" << std::endl;
        return;
    }

    std::string path = requestedPath;
#ifdef _WIN32
    if (path.empty()) {
        static const char filter[] =
            "STL File (*.stl)\0*.stl\0"
            "All Files (*.*)\0*.*\0\0";
        path = ShowWindowsFileDialog(
            filter,
            OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_EXPLORER,
            "stl");
        if (path.empty()) {
            return;
        }
    }
#endif

    std::ofstream out(path);
    if (!out.is_open()) {
        std::cout << "Failed to open file for writing: " << path << std::endl;
        return;
    }

    out << "solid ExportedMesh\n";

    for (const auto& mesh : meshes) {
        const glm::vec3 meshPosition = mesh->GetPosition();
        const auto& editableVertices = mesh->GetEditableVertices();
        const auto& logicalFaces = mesh->GetFaces();

        for (const auto& face : logicalFaces) {
            for (std::size_t i = 1; i + 1 < face.size(); ++i) {
                const glm::vec3 v0 = editableVertices[face[0]] + meshPosition;
                const glm::vec3 v1 = editableVertices[face[i]] + meshPosition;
                const glm::vec3 v2 = editableVertices[face[i + 1]] + meshPosition;
                const glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

                out << "facet normal " << normal.x << " " << normal.y << " " << normal.z << "\n";
                out << "  outer loop\n";
                out << "    vertex " << v0.x << " " << v0.y << " " << v0.z << "\n";
                out << "    vertex " << v1.x << " " << v1.y << " " << v1.z << "\n";
                out << "    vertex " << v2.x << " " << v2.y << " " << v2.z << "\n";
                out << "  endloop\n";
                out << "endfacet\n";
            }
        }
    }

    out << "endsolid ExportedMesh\n";
    std::cout << "Exported " << meshes.size() << " mesh(es) to: " << path << std::endl;
}
