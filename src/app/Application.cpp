#include "app/Application.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>

// Статическая функция для обработки скролла
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    void* userPtr = glfwGetWindowUserPointer(window);
    if (userPtr) {
        Application* app = static_cast<Application*>(userPtr);
        app->GetCamera().Zoom(static_cast<float>(yoffset));
    }
}

Application::~Application() = default;

Application::Application()
    : window(1920, 1080, "CatMesh"), ui(window, this),
    lastMouseX(0), lastMouseY(0), mousePressed(false)
{
    camera.Update();
    AddDefaultCube();

    // Устанавливаем user pointer и scroll callback
    GLFWwindow* nativeWindow = window.GetNativeWindow();
    glfwSetWindowUserPointer(nativeWindow, this);
    glfwSetScrollCallback(nativeWindow, scroll_callback);
}

void Application::AddDefaultCube() {
    auto mesh = std::make_unique<Mesh>();
    mesh->LoadFromFile("");
    mesh->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    mesh->SetColor(glm::vec3(0.5f, 0.5f, 0.5f)); // серый
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

void Application::HandleInput() {
    auto* glfwWindow = window.GetNativeWindow();

    double x, y;
    glfwGetCursorPos(glfwWindow, &x, &y);

    // Вращение камеры на ПКМ
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
    // Перемещение объекта на ЛКМ
    else if (glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && selectedMesh) {
        if (!mousePressed) {
            lastMouseX = x;
            lastMouseY = y;
            mousePressed = true;
        }
        else {
            float deltaX = static_cast<float>((x - lastMouseX) * 0.01f);
            float deltaY = static_cast<float>((y - lastMouseY) * 0.01f);

            glm::vec3 pos = selectedMesh->GetPosition();
            pos.x += deltaX;
            pos.y += deltaY;
            selectedMesh->SetPosition(pos);

            lastMouseX = x;
            lastMouseY = y;
        }
    }
    else {
        mousePressed = false;
    }
}

void Application::Run() {
    glEnable(GL_DEPTH_TEST);

    while (!window.ShouldClose()) {
        HandleInput();
        window.PollEvents();

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

void Application::ExportSTL() {
    if (meshes.empty()) {
        std::cout << "No meshes to export!" << std::endl;
        return;
    }

    std::cout << "Export STL - Enter filename: ";
    std::string filename;
    std::cin >> filename;
    filename += ".stl";

    std::ofstream file(filename);
    if (file.is_open()) {
        file << "solid ExportedMesh\n";
        file << "endsolid ExportedMesh\n";
        file.close();
        std::cout << "Exported to: " << filename << std::endl;
    }
}