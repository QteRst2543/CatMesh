#include "app/Application.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "core/Mesh.h"
#include "core/Grid.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <unordered_map>
#include <cmath>
#include <float.h> 

// Статическая функция для обработки скролла
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    void* userPtr = glfwGetWindowUserPointer(window);
    if (userPtr) {
        Application* app = static_cast<Application*>(userPtr);
        app->GetCamera().Zoom(static_cast<float>(yoffset));
    }
}

// Проверка на нажатие кнопки мыши не на ui
bool IsKeyPressedOnce(GLFWwindow* window, int key) {
    static std::unordered_map<int, bool> wasPressed;
    bool pressed = glfwGetKey(window, key) == GLFW_PRESS;
    bool result = pressed && !wasPressed[key];
    wasPressed[key] = pressed;
    return result;
}

Application::~Application() = default;

Application::Application()
    : window(1920, 1080, "CatMesh"), ui(window, this),
    lastMouseX(0), lastMouseY(0), mousePressed(false), dragStartMouseX(0), dragStartMouseY(0),
    isDraggingElement(false)
{
    camera.Update();
    AddDefaultCube();

    // Устанавливаем scroll callback
    GLFWwindow* nativeWindow = window.GetNativeWindow();
    glfwSetWindowUserPointer(nativeWindow, this);
    glfwSetScrollCallback(nativeWindow, scroll_callback);
}

void Application::AddDefaultCube() {
    auto mesh = std::make_unique<Mesh>();
    mesh->LoadFromFile("");
    mesh->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    mesh->SetColor(glm::vec3(0.5f, 0.5f, 0.5f));
    mesh->SetEditMode(Mesh::EditMode::Object);
    selectedMesh = mesh.get();
    meshes.push_back(std::move(mesh));
}

void Application::DeleteSelectedMesh() {
    if (!selectedMesh) return;
    auto it = std::find_if(meshes.begin(), meshes.end(),
        [this](auto& ptr) { return ptr.get() == selectedMesh; });
    if (it != meshes.end()) {
        meshes.erase(it);
        selectedMesh = meshes.empty() ? nullptr : meshes.front().get();
    }
}

void Application::ExecuteCommand(std::unique_ptr<Command> cmd) {
    cmd->Execute();
    undoStack.push(std::move(cmd));
    while (!redoStack.empty()) redoStack.pop();
}

void Application::Undo() {
    if (undoStack.empty()) return;
    auto cmd = std::move(undoStack.top());
    undoStack.pop();
    cmd->Undo();
    redoStack.push(std::move(cmd));
}

void Application::Redo() {
    if (redoStack.empty()) return;
    auto cmd = std::move(redoStack.top());
    redoStack.pop();
    cmd->Execute();
    undoStack.push(std::move(cmd));
}

Mesh* Application::AddMesh(std::unique_ptr<Mesh> mesh) {
    Mesh* raw = mesh.get();
    selectedMesh = raw;
    meshes.push_back(std::move(mesh));
    return raw;
}

void Application::RemoveMesh(Mesh* mesh) {
    auto it = std::find_if(meshes.begin(), meshes.end(),
        [mesh](auto& ptr) { return ptr.get() == mesh; });
    if (it != meshes.end()) {
        meshes.erase(it);
        if (selectedMesh == mesh)
            selectedMesh = meshes.empty() ? nullptr : meshes.front().get();
    }
}

glm::vec3 Application::GetRayFromMouse(double mouseX, double mouseY, int screenWidth, int screenHeight) {
    // Нормализованные координаты устройства (-1 до 1)
    float ndcX = (2.0f * mouseX) / screenWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY) / screenHeight;

    glm::mat4 proj = camera.GetProjectionMatrix((float)screenWidth / screenHeight);
    glm::mat4 view = camera.GetViewMatrix();

    // Clip space
    glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);

    // Eye space
    glm::vec4 rayEye = glm::inverse(proj) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    // World space
    glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

    return rayWorld;
}

glm::vec3 Application::GetPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& planePoint, const glm::vec3& planeNormal) {
    float denominator = glm::dot(planeNormal, rayDir);
    if (fabs(denominator) < 0.0001f) {
        return planePoint; // Луч параллелен плоскости
    }

    float t = glm::dot(planePoint - rayOrigin, planeNormal) / denominator;
    if (t < 0) {
        return planePoint;
    }

    return rayOrigin + rayDir * t;
}

void Application::HandleInput() {
    auto* glfwWindow = window.GetNativeWindow();

    int screenWidth, screenHeight;
    glfwGetFramebufferSize(glfwWindow, &screenWidth, &screenHeight);

    double x, y;
    glfwGetCursorPos(glfwWindow, &x, &y);

    // Обновляем позицию курсора в Camera для ray picking
    camera.SetMousePosition(static_cast<float>(x), static_cast<float>(y), screenWidth, screenHeight);

    if (ImGui::GetIO().WantCaptureMouse)
        return;

    // ЛКМ - выбор и перемещение
    if (glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!mousePressed) {
            // Начало клика - пробуем выделить объект или элемент
            lastMouseX = x;
            lastMouseY = y;
            mousePressed = true;
            isDraggingElement = false;
            dragStartMouseX = x;
            dragStartMouseY = y;

            if (selectedMesh) {
                // Получаем луч из камеры через позицию мыши
                glm::vec3 rayDir = GetRayFromMouse(x, y, screenWidth, screenHeight);
                glm::vec3 cameraPos = camera.GetPosition();

                if (selectedMesh->GetEditMode() != Mesh::EditMode::Object) {
                    // В режиме редактирования - выделяем грань/вершину/ребро
                    selectedMesh->UpdateSelectionFromMouse(cameraPos, rayDir);
                }
                else {
                    // В режиме объекта - выделяем меш через bounding box
                    // (пока просто проверяем клик по любому мешу)
                    for (auto& mesh : meshes) {
                        if (mesh->IntersectRay(cameraPos, rayDir)) {
                            selectedMesh = mesh.get();
                            break;
                        }
                    }
                }
            }
        }
        else {
            // Движение мыши при зажатой кнопке
            float deltaX = static_cast<float>(x - lastMouseX);
            float deltaY = static_cast<float>(y - lastMouseY);

            // Проверяем, достаточно ли далеко переместилась мышь для начала drag
            if (!isDraggingElement && (fabs(deltaX) > 5.0f || fabs(deltaY) > 5.0f)) {
                isDraggingElement = true;
            }

            if (isDraggingElement && selectedMesh) {
                if (selectedMesh->GetEditMode() == Mesh::EditMode::Object) {
                    // Перемещение объекта по плоскости XZ с привязкой к сетке
                    glm::vec3 rayDir = GetRayFromMouse(x, y, screenWidth, screenHeight);
                    glm::vec3 cameraPos = camera.GetPosition();

                    // Плоскость на уровне Y=0 для перемещения
                    glm::vec3 planePoint(0.0f, 0.0f, 0.0f);
                    glm::vec3 planeNormal(0.0f, 1.0f, 0.0f);

                    glm::vec3 intersection = GetPlaneIntersection(cameraPos, rayDir, planePoint, planeNormal);

                    // Привязка к сетке (snap)
                    if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
                        intersection.x = roundf(intersection.x);
                        intersection.z = roundf(intersection.z);
                    }

                    selectedMesh->SetPosition(intersection);
                }
                else if (selectedMesh->GetEditMode() == Mesh::EditMode::Face && selectedMesh->GetSelectedFace() >= 0) {
                    // Перемещение грани по лучу мыши
                    glm::vec3 rayDir = GetRayFromMouse(x, y, screenWidth, screenHeight);
                    glm::vec3 cameraPos = camera.GetPosition();

                    // Получаем центр выбранной грани
                    glm::vec3 faceCenter = selectedMesh->GetFaceCenter(selectedMesh->GetSelectedFace());
                    faceCenter += selectedMesh->GetPosition();

                    // Создаем плоскость, проходящую через центр грани и перпендикулярную лучу камеры
                    glm::vec3 planeNormal = rayDir;
                    glm::vec3 intersection = GetPlaneIntersection(cameraPos, rayDir, faceCenter, planeNormal);

                    // Вычисляем дельту перемещения
                    static glm::vec3 lastIntersection = intersection;
                    if (lastIntersection != glm::vec3(0.0f)) {
                        glm::vec3 delta = intersection - lastIntersection;
                        selectedMesh->DragSelectedElement(delta);
                    }
                    lastIntersection = intersection;
                }
                else if (selectedMesh->GetEditMode() == Mesh::EditMode::Vertex && selectedMesh->GetSelectedVertex() >= 0) {
                    // Перемещение вершины
                    glm::vec3 rayDir = GetRayFromMouse(x, y, screenWidth, screenHeight);
                    glm::vec3 cameraPos = camera.GetPosition();

                    glm::vec3 vertexPos = selectedMesh->GetVertexPosition(selectedMesh->GetSelectedVertex());
                    vertexPos += selectedMesh->GetPosition();

                    glm::vec3 planeNormal = rayDir;
                    glm::vec3 intersection = GetPlaneIntersection(cameraPos, rayDir, vertexPos, planeNormal);

                    static glm::vec3 lastIntersection = intersection;
                    if (lastIntersection != glm::vec3(0.0f)) {
                        glm::vec3 delta = intersection - lastIntersection;
                        selectedMesh->DragSelectedElement(delta);
                    }
                    lastIntersection = intersection;
                }
            }

            lastMouseX = x;
            lastMouseY = y;
        }
    }
    else {
        // Кнопка отпущена - сбрасываем состояние
        mousePressed = false;
        isDraggingElement = false;
    }

    // ПКМ - вращение камеры
    if (glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        if (!mousePressed) {
            lastMouseX = x;
            lastMouseY = y;
            mousePressed = true;
        }
        else {
            float deltaX = static_cast<float>(x - lastMouseX);
            float deltaY = static_cast<float>(y - lastMouseY);
            camera.Rotate(deltaX, deltaY);
            lastMouseX = x;
            lastMouseY = y;
        }
    }

    // Клавиши для переключения режимов редактирования
    if (IsKeyPressedOnce(glfwWindow, GLFW_KEY_1) && selectedMesh) {
        selectedMesh->SetEditMode(Mesh::EditMode::Object);
        std::cout << "Switched to Object Mode" << std::endl;
    }
    if (IsKeyPressedOnce(glfwWindow, GLFW_KEY_2) && selectedMesh) {
        selectedMesh->SetEditMode(Mesh::EditMode::Vertex);
        std::cout << "Switched to Vertex Mode" << std::endl;
    }
    if (IsKeyPressedOnce(glfwWindow, GLFW_KEY_3) && selectedMesh) {
        selectedMesh->SetEditMode(Mesh::EditMode::Edge);
        std::cout << "Switched to Edge Mode" << std::endl;
    }
    if (IsKeyPressedOnce(glfwWindow, GLFW_KEY_4) && selectedMesh) {
        selectedMesh->SetEditMode(Mesh::EditMode::Face);
        std::cout << "Switched to Face Mode" << std::endl;
    }

    // Удаление выделенного (Delete)
    if (IsKeyPressedOnce(glfwWindow, GLFW_KEY_DELETE)) {
        DeleteSelectedMesh();
    }
}

void Application::Run() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


    // Создаем сетку
    Grid grid;

    while (!window.ShouldClose()) {
        HandleInput();
        window.PollEvents();

        // Проверка клавиш для Undo/Redo
        if (IsKeyPressedOnce(window.GetNativeWindow(), GLFW_KEY_Y) &&
            (glfwGetKey(window.GetNativeWindow(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                glfwGetKey(window.GetNativeWindow(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)) {
            Redo();
        }

        if (IsKeyPressedOnce(window.GetNativeWindow(), GLFW_KEY_Z) &&
            (glfwGetKey(window.GetNativeWindow(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                glfwGetKey(window.GetNativeWindow(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)) {
            Undo();
        }

        ui.NewFrame();
        ui.Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window.GetNativeWindow(), &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspect = static_cast<float>(display_w) / static_cast<float>(display_h);
        auto view = camera.GetViewMatrix();
        auto proj = camera.GetProjectionMatrix(aspect);

        // Рисуем сетку
        grid.Draw(view, proj);

        // Рисуем оси координат
        grid.DrawAxes(view, proj);

        // Рисуем все меши
        for (auto& m : meshes) {
            m->Draw(view, proj);
        }

        ui.RenderDrawData();
        window.SwapBuffers();
    }
}

void Application::OpenFile() {
    std::cout << "Open file - Enter filename: ";
    std::string filename;
    std::cin >> filename;
    meshes.clear();
    AddDefaultCube();
    std::cout << "Opened: " << filename << std::endl;
}

void Application::SaveFile() {
    std::cout << "Save file - Enter filename: ";
    std::string filename;
    std::cin >> filename;
    std::cout << "Saved: " << filename << std::endl;
}

void Application::ExportSTL(const std::string& path) {
    if (meshes.empty()) {
        std::cout << "No meshes to export!" << std::endl;
        return;
    }

    std::ofstream out(path);
    if (!out.is_open()) {
        std::cout << "Failed to open file for writing: " << path << std::endl;
        return;
    }

    out << "solid ExportedMesh\n";

    // Проходим по всем мешам
    for (const auto& meshPtr : meshes) {
        const std::vector<float>& vertices = meshPtr->GetVertices();
        const std::vector<unsigned int>& indices = meshPtr->GetIndices();

        // Получаем позицию меша для трансформации вершин
        glm::vec3 meshPos = meshPtr->GetPosition();

        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            unsigned int i0 = indices[i];
            unsigned int i1 = indices[i + 1];
            unsigned int i2 = indices[i + 2];

            // Создаем вершины с учетом позиции меша
            glm::vec3 v0(
                vertices[i0 * 3] + meshPos.x,
                vertices[i0 * 3 + 1] + meshPos.y,
                vertices[i0 * 3 + 2] + meshPos.z
            );

            glm::vec3 v1(
                vertices[i1 * 3] + meshPos.x,
                vertices[i1 * 3 + 1] + meshPos.y,
                vertices[i1 * 3 + 2] + meshPos.z
            );

            glm::vec3 v2(
                vertices[i2 * 3] + meshPos.x,
                vertices[i2 * 3 + 1] + meshPos.y,
                vertices[i2 * 3 + 2] + meshPos.z
            );

            // Вычисляем нормаль
            glm::vec3 n = glm::normalize(glm::cross(v1 - v0, v2 - v0));

            // Записываем треугольник в STL формате
            out << "facet normal " << n.x << " " << n.y << " " << n.z << "\n";
            out << "  outer loop\n";
            out << "    vertex " << v0.x << " " << v0.y << " " << v0.z << "\n";
            out << "    vertex " << v1.x << " " << v1.y << " " << v1.z << "\n";
            out << "    vertex " << v2.x << " " << v2.y << " " << v2.z << "\n";
            out << "  endloop\n";
            out << "endfacet\n";
        }
    }

    out << "endsolid ExportedMesh\n";
    out.close();

    std::cout << "Exported " << meshes.size() << " mesh(es) to: " << path << std::endl;
}