#include "app/Application.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "core/Mesh.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <unordered_map>


// Статическая функция для обработки скролла
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    void* userPtr = glfwGetWindowUserPointer(window);
    if (userPtr) {
        Application* app = static_cast<Application*>(userPtr);
        app->GetCamera().Zoom(static_cast<float>(yoffset));
    }
}

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
    lastMouseX(0), lastMouseY(0), mousePressed(false)
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

    if (ImGui::GetIO().WantCaptureMouse)
        return;
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
    while(!window.ShouldClose()) {
        window.PollEvents();
        if (IsKeyPressedOnce(window.GetNativeWindow(), GLFW_KEY_Y)) 
            Application::Redo();
        
        if (IsKeyPressedOnce(window.GetNativeWindow(), GLFW_KEY_Z))
            Application::Undo();
   
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

            // Записываем треугольник в STL формате (обратите внимание на пробелы!)
            out << "facet normal " << n.x << " " << n.y << " " << n.z << "\n";
            out << "  outer loop\n";
            out << "    vertex " << v0.x << " " << v0.y << " " << v0.z << "\n";
            out << "    vertex " << v1.x << " " << v1.y << " " << v1.z << "\n";
            out << "    vertex " << v2.x << " " << v2.y << " " << v2.z << "\n";
            out << "  endloop\n";
            out << "endfacet\n";  // Должно быть "endfacet", а не "endFacet"
        }
    }

    out << "endsolid ExportedMesh\n";
    out.close();

    std::cout << "Exported " << meshes.size() << " mesh(es) to: " << path << std::endl;
}