#include "app/Application.h"
#include "app/FileDialog.h"
#include "core/Mesh.h"
#include "core/Grid.h"
#include "core/Shader.h"
#include "imgui.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
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

namespace {

} // namespace

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
// Функция открытия диалогового окна сохранена в src/app/FileDialog.cpp
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

    InitShadowMapping();
}

void Application::InitShadowMapping() {
    // Create shadow shader program
    std::string shadowVertexCode = LoadFile("shaders/shadow.vert");
    std::string shadowFragmentCode = LoadFile("shaders/shadow.frag");

    if (shadowVertexCode.empty() || shadowFragmentCode.empty()) {
        std::cerr << "Failed to load shadow shader files!" << std::endl;
        shadowFBO = 0;
        shadowMap = 0;
        shadowShaderProgram = 0;
        return;
    }

    unsigned int shadowVertexShader = CompileShader(GL_VERTEX_SHADER, shadowVertexCode);
    unsigned int shadowFragmentShader = CompileShader(GL_FRAGMENT_SHADER, shadowFragmentCode);

    if (!shadowVertexShader || !shadowFragmentShader) {
        std::cerr << "Shadow shader compilation failed!" << std::endl;
        shadowFBO = 0;
        shadowMap = 0;
        shadowShaderProgram = 0;
        return;
    }

    shadowShaderProgram = glCreateProgram();
    glAttachShader(shadowShaderProgram, shadowVertexShader);
    glAttachShader(shadowShaderProgram, shadowFragmentShader);
    glLinkProgram(shadowShaderProgram);

    int success = 0;
    glGetProgramiv(shadowShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shadowShaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shadow shader linking failed: " << infoLog << std::endl;
        glDeleteProgram(shadowShaderProgram);
        shadowShaderProgram = 0;
        shadowFBO = 0;
        shadowMap = 0;
        glDeleteShader(shadowVertexShader);
        glDeleteShader(shadowFragmentShader);
        return;
    }

    glDeleteShader(shadowVertexShader);
    glDeleteShader(shadowFragmentShader);

    // Create shadow FBO and texture
    glGenFramebuffers(1, &shadowFBO);
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 2048, 2048, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Shadow framebuffer is not complete: " << status << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
    selectedMesh = nullptr;
    selectedEntityType = SelectedEntityType::None;
}

void Application::DeleteSelectedObject() {
    if (selectedEntityType == SelectedEntityType::Mesh) {
        DeleteSelectedMesh();
    }
    else if (selectedEntityType == SelectedEntityType::Light) {
        light.enabled = false;
        selectedEntityType = SelectedEntityType::None;
        selectedMesh = nullptr;
    }
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
        selectedMesh = nullptr;
        selectedEntityType = SelectedEntityType::None;
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

    glm::vec3 moveDelta(0.0f);
    float moveSpeed = 4.0f * deltaTime;
    if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(glfwWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
        moveSpeed *= 2.0f;
    }

    // Get camera direction vectors (with full Y component for vertical movement)
    glm::vec3 forward = camera.GetForward();
    glm::vec3 right = camera.GetRight();

    if (glfwGetKey(glfwWindow, GLFW_KEY_W) == GLFW_PRESS) {
        moveDelta += forward * moveSpeed;
    }
    if (glfwGetKey(glfwWindow, GLFW_KEY_S) == GLFW_PRESS) {
        moveDelta -= forward * moveSpeed;
    }
    if (glfwGetKey(glfwWindow, GLFW_KEY_D) == GLFW_PRESS) {
        moveDelta += right * moveSpeed;
    }
    if (glfwGetKey(glfwWindow, GLFW_KEY_A) == GLFW_PRESS) {
        moveDelta -= right * moveSpeed;
    }

    if (glm::length(moveDelta) > 0.0001f) {
        camera.MoveTarget(moveDelta);
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

    if (selectedMesh && selectedMesh->GetEditMode() == Mesh::EditMode::Face) {
        selectedMesh->UpdateSelectionFromMouse(cameraPos, rayDir);
    }

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

    auto isLightClicked = [&]() -> bool {
        if (!light.enabled) return false;
        // Simple sphere intersection for light
        const float lightRadius = 0.5f; // arbitrary radius for clicking
        const glm::vec3 toLight = light.position - cameraPos;
        const float distToLight = glm::length(toLight);
        if (distToLight > 10.0f) return false; // too far
        const float cosAngle = glm::dot(glm::normalize(toLight), rayDir);
        if (cosAngle < 0.95f) return false; // not facing
        const float sinAngle = std::sqrt(1.0f - cosAngle * cosAngle);
        const float distToRay = distToLight * sinAngle;
        return distToRay < lightRadius;
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
                extrudeOperationActive = false;
                extrudePerformed = false;
                extrudeFaceIndex = -1;

                Mesh* clickedMesh = findClosestMeshUnderCursor();
                bool clickedLight = isLightClicked();

                if (clickedMesh && clickedMesh->GetEditMode() == Mesh::EditMode::Face) {
                    clickedMesh->UpdateSelectionFromMouse(cameraPos, rayDir);
                }

                if (clickedLight) {
                    selectedEntityType = SelectedEntityType::Light;
                    selectedMesh = nullptr;
                } else if (clickedMesh) {
                    selectedMesh = clickedMesh;
                    selectedEntityType = SelectedEntityType::Mesh;
                    if (GetToolMode() == ToolMode::Extrude &&
                        selectedMesh->GetEditMode() == Mesh::EditMode::Face &&
                        selectedMesh->GetSelectedFace() >= 0) {
                        extrudeOperationActive = true;
                        extrudePerformed = false;
                        extrudeFaceIndex = selectedMesh->GetSelectedFace();
                    }
                } else {
                    selectedEntityType = SelectedEntityType::None;
                    selectedMesh = nullptr;
                }

                if (selectedEntityType == SelectedEntityType::Light) {
                    dragPlanePoint = light.position;
                    dragPlaneNormal = camera.GetForward();
                    hasDragPlane = true;

                    const glm::vec3 intersection = GetPlaneIntersection(cameraPos, rayDir, dragPlanePoint, dragPlaneNormal);
                    objectDragOffset = light.position - intersection;
                    hasObjectDragOffset = true;
                }
                else if (selectedMesh && selectedMesh == clickedMesh) {
                    if (selectedMesh->GetEditMode() == Mesh::EditMode::Face || selectedMesh->GetEditMode() == Mesh::EditMode::Vertex) {
                        selectedMesh->UpdateSelectionFromMouse(cameraPos, rayDir);
                    }

                    if (GetToolMode() == ToolMode::Split) {
                        if (selectedMesh->GetEditMode() == Mesh::EditMode::Face && selectedMesh->GetSelectedFace() >= 0) {
                            selectedMesh->SplitSelectedFace();
                        } else if (selectedMesh->GetEditMode() == Mesh::EditMode::Vertex && selectedMesh->GetSelectedVertex() >= 0) {
                            selectedMesh->SplitSelectedVertex();
                        }
                    }
                    else if (selectedMesh->GetEditMode() == Mesh::EditMode::Face && selectedMesh->GetSelectedFace() >= 0) {
                        dragPlanePoint = selectedMesh->GetFaceCenter(selectedMesh->GetSelectedFace()) + selectedMesh->GetPosition();
                        dragPlaneNormal = camera.GetForward();
                        hasDragPlane = true;
                    }
                    else {
                        dragPlanePoint = selectedMesh->GetPosition();
                        dragPlaneNormal = camera.GetForward();
                        hasDragPlane = true;

                        const glm::vec3 intersection = GetPlaneIntersection(cameraPos, rayDir, dragPlanePoint, dragPlaneNormal);
                        objectDragOffset = selectedMesh->GetPosition() - intersection;
                        hasObjectDragOffset = true;
                    }
                }
            }
            else {
                const float totalDeltaX = static_cast<float>(mouseX - dragStartMouseX);
                const float totalDeltaY = static_cast<float>(mouseY - dragStartMouseY);

                if (!isDraggingElement && (std::fabs(totalDeltaX) > 5.0f || std::fabs(totalDeltaY) > 5.0f)) {
                    isDraggingElement = true;
                }

                if (isDraggingElement && (selectedMesh || selectedEntityType == SelectedEntityType::Light)) {
                    const ToolMode toolMode = GetToolMode();

                    if (toolMode == ToolMode::Move && selectedEntityType == SelectedEntityType::Mesh && selectedMesh) {
                        if (!hasDragPlane) {
                            dragPlanePoint = selectedMesh->GetPosition();
                            dragPlaneNormal = camera.GetForward();
                            hasDragPlane = true;
                        }

                        const glm::vec3 intersection = GetPlaneIntersection(cameraPos, rayDir, dragPlanePoint, dragPlaneNormal);

                        if (!hasLastDragIntersection) {
                            lastDragIntersection = intersection;
                            hasLastDragIntersection = true;
                        } else {
                            const glm::vec3 delta = intersection - lastDragIntersection;
                            if (glm::length(delta) > 0.00001f) {
                                selectedMesh->TranslateByVector(delta);
                            }
                            lastDragIntersection = intersection;
                        }
                    }
                    else if (toolMode == ToolMode::Extrude && selectedEntityType == SelectedEntityType::Mesh && selectedMesh) {
                        if (selectedMesh->GetEditMode() != Mesh::EditMode::Face || selectedMesh->GetSelectedFace() < 0) {
                            // Экструзия доступна только в режиме редактирования граней и при наличии наведения.
                        } else {
                            if (!hasDragPlane) {
                                dragPlanePoint = selectedMesh->GetFaceCenter(selectedMesh->GetSelectedFace()) + selectedMesh->GetPosition();
                                dragPlaneNormal = camera.GetForward();
                                hasDragPlane = true;
                            }

                            const glm::vec3 intersection = GetPlaneIntersection(cameraPos, rayDir, dragPlanePoint, dragPlaneNormal);

                            if (!hasLastDragIntersection) {
                                lastDragIntersection = intersection;
                                hasLastDragIntersection = true;
                            } else {
                                const glm::vec3 delta = intersection - lastDragIntersection;
                                if (glm::length(delta) > 0.00001f) {
                                    const glm::vec3 faceNormal = selectedMesh->GetFaceNormal(selectedMesh->GetSelectedFace());
                                    const float proj = glm::dot(delta, faceNormal);
                                    if (proj > 0.0001f) {
                                        if (!extrudePerformed && extrudeOperationActive && selectedMesh->GetSelectedFace() == extrudeFaceIndex) {
                                            selectedMesh->ExtrudeSelectedAlongDirection(faceNormal * proj);
                                            extrudePerformed = true;
                                        } else if (extrudePerformed) {
                                            selectedMesh->DragSelectedElement(faceNormal * proj);
                                        }
                                        dragPlanePoint += faceNormal * proj;
                                    }
                                }
                                lastDragIntersection = intersection;
                            }
                        }
                    }
                    else if (toolMode == ToolMode::Rotate && selectedEntityType == SelectedEntityType::Mesh && selectedMesh) {
                        glm::vec3 axis(0.0f);
                        axis[rotationAxis] = 1.0f;

                        if (!hasDragPlane) {
                            if (selectedMesh->GetEditMode() == Mesh::EditMode::Face && selectedMesh->GetSelectedFace() >= 0) {
                                dragPlanePoint = selectedMesh->GetFaceCenter(selectedMesh->GetSelectedFace()) + selectedMesh->GetPosition();
                            } else if (selectedMesh->GetEditMode() == Mesh::EditMode::Vertex && selectedMesh->GetSelectedVertex() >= 0) {
                                dragPlanePoint = selectedMesh->GetVertexPosition(selectedMesh->GetSelectedVertex()) + selectedMesh->GetPosition();
                            } else {
                                dragPlanePoint = selectedMesh->GetPosition();
                            }
                            dragPlaneNormal = axis;
                            hasDragPlane = true;
                        }

                        const glm::vec3 intersection = GetPlaneIntersection(cameraPos, rayDir, dragPlanePoint, dragPlaneNormal);

                        if (!hasLastDragIntersection) {
                            lastDragIntersection = intersection;
                            hasLastDragIntersection = true;
                        } else {
                            const glm::vec3 center = dragPlanePoint;
                            const glm::vec3 v0 = lastDragIntersection - center;
                            const glm::vec3 v1 = intersection - center;
                            if (glm::length(v0) > 0.00001f && glm::length(v1) > 0.00001f) {
                                float angle = atan2(glm::dot(axis, glm::cross(v0, v1)), glm::dot(v0, v1));
                                if (std::fabs(angle) > 0.00001f) {
                                    selectedMesh->RotateSelected(axis, angle);
                                }
                            }
                            lastDragIntersection = intersection;
                        }
                    }
                    else if (selectedEntityType == SelectedEntityType::Light) {
                        if (!hasDragPlane) {
                            dragPlanePoint = light.position;
                            dragPlaneNormal = camera.GetForward();
                            hasDragPlane = true;
                        }

                        glm::vec3 intersection = GetPlaneIntersection(cameraPos, rayDir, dragPlanePoint, dragPlaneNormal);

                        if (!hasObjectDragOffset) {
                            objectDragOffset = light.position - intersection;
                            hasObjectDragOffset = true;
                        }

                        glm::vec3 newPosition = intersection + objectDragOffset;
                        if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                            glfwGetKey(glfwWindow, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
                            newPosition.x = std::round(newPosition.x);
                            newPosition.y = std::round(newPosition.y);
                            newPosition.z = std::round(newPosition.z);
                        }

                        light.position = newPosition;
                        dragPlanePoint = newPosition;
                    }
                    else if (selectedMesh->GetEditMode() == Mesh::EditMode::Object) {
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
        if (displayHeight <= 0) {
            displayHeight = 1;
        }
        glViewport(0, 0, displayWidth, displayHeight);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect = static_cast<float>(displayWidth) / static_cast<float>(displayHeight);
        const glm::mat4 view = camera.GetViewMatrix();
        const glm::mat4 projection = camera.GetProjectionMatrix(aspect);

        // Shadow mapping
float near_plane = 0.1f, far_plane = 30.0f;
float orthoSize = 12.0f; // Увеличиваем размер
glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, near_plane, far_plane);
glm::mat4 lightView = glm::lookAt(light.position, light.position + light.direction, glm::vec3(0.0, 1.0, 0.0));
lightSpaceMatrix = lightProjection * lightView;

        // 1. Render depth of scene to texture (from light's perspective)
       if (shadowShaderProgram && shadowFBO && shadowMap) {
    glViewport(0, 0, 2048, 2048);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // Важно: отключаем запись в цветовой буфер
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    
    glUseProgram(shadowShaderProgram);
    for (auto& mesh : meshes) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), mesh->GetPosition());
        glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "lightSpaceMatrix"), 
                          1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "model"), 
                          1, GL_FALSE, glm::value_ptr(model));
        
        glBindVertexArray(mesh->GetVAO());
        glDrawElements(GL_TRIANGLES, mesh->GetVertexCount(), GL_UNSIGNED_INT, 0);
    }
    
    // Включаем обратно запись цвета
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
        // Reset viewport
        glViewport(0, 0, displayWidth, displayHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
                light.outerCutoffDegrees,
                lightSpaceMatrix,
                shadowMap);
        }

        if (light.enabled) {
            glDisable(GL_DEPTH_TEST);
            grid.DrawAxes(view, projection, light.position, 0.25f);
            glEnable(GL_DEPTH_TEST);
        }

        if (showLightRays && light.enabled) {
            grid.DrawLightCone(view, projection, light.position, light.direction,
                light.innerCutoffDegrees, light.outerCutoffDegrees);
        }

        if (selectedMesh) {
            glDisable(GL_DEPTH_TEST);
            grid.DrawAxes(view, projection, selectedMesh->GetPosition(), 0.75f);
            selectedMesh->DrawSelectedFaceOutline(view, projection);
            if (GetToolMode() == ToolMode::Rotate) {
                grid.DrawRotationGizmo(view, projection, selectedMesh->GetPosition(), rotationAxis);
            }
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
    else if (extension == ".obj") {
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
    else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp") {
        auto mesh = std::make_shared<Mesh>();
        if (!mesh->SetTexturedPlane(path)) {
            return;
        }

        mesh->SetEditMode(Mesh::EditMode::Object);
        ClearHistory();
        selectedMesh = mesh.get();
        meshes.push_back(mesh);
        std::cout << "Imported image plane: " << path << std::endl;
    }
    else {
        std::cerr << "Unsupported file type: " << extension << std::endl;
    }
#else
    std::cerr << "OpenFile is only implemented with native dialogs on Windows." << std::endl;
#endif
}

void Application::ImportImage() {
#ifdef _WIN32
    static const char filter[] =
        "Image Files (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0"
        "All Files (*.*)\0*.*\0\0";

    const std::string path = ShowWindowsFileDialog(
        filter,
        OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER,
        nullptr);
    if (path.empty()) {
        return;
    }

    auto mesh = std::make_shared<Mesh>();
    if (!mesh->SetTexturedPlane(path)) {
        std::cerr << "Failed to import image: " << path << std::endl;
        return;
    }

    mesh->SetEditMode(Mesh::EditMode::Object);
    ClearHistory();
    selectedMesh = mesh.get();
    meshes.push_back(mesh);
    std::cout << "Imported image plane: " << path << std::endl;
#else
    std::cerr << "ImportImage is only implemented on Windows." << std::endl;
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

void Application::SelectMesh(Mesh* mesh) {
    selectedMesh = mesh;
    selectedEntityType = SelectedEntityType::Mesh;
}

void Application::SelectLight() {
    selectedMesh = nullptr;
    selectedEntityType = SelectedEntityType::Light;
}

void Application::AddLightSource() {
    glm::vec3 lightPos = glm::vec3(3.0f, 4.0f, 3.0f);
    if (selectedMesh && selectedEntityType == SelectedEntityType::Mesh) {
        lightPos = selectedMesh->GetPosition() + glm::vec3(3.0f, 4.0f, 3.0f);
    }

    light.position = lightPos;
    light.enabled = true;
    selectedMesh = nullptr;
    selectedEntityType = SelectedEntityType::None;
}

