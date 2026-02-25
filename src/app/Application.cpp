#include "app/Application.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

Application::~Application() = default;

Application::Application()
    : window(1920, 1080, "CatMesh0.0.1"), ui(window),
    lastMouseX(0), lastMouseY(0), mousePressed(false)
{
    camera.Update();

    mesh = new Mesh();
    if (!mesh->LoadFromFile("")) {  // пустой файл = куб пока
        std::cerr << "Failed to load mesh" << std::endl;
    }
    mesh->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
}

void Application::HandleInput() {
    auto* glfwWindow = window.GetNativeWindow();

    if (glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        double x, y;
        glfwGetCursorPos(glfwWindow, &x, &y);

        if (!mousePressed) {
            lastMouseX = x;
            lastMouseY = y;
            mousePressed = true;
        }
        else {
            float deltaX = x - lastMouseX;
            float deltaY = y - lastMouseY;
            camera.Rotate(deltaX, deltaY);
            lastMouseX = x;
            lastMouseY = y;
        }
    }
    else {
        mousePressed = false;
    }

    // Zoom с колёсика
    // TODO: добавить обработку scroll callback
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

        if (mesh) {
            mesh->Draw(view, proj);
        }

        ui.RenderDrawData();

        window.SwapBuffers();
    }

    delete mesh;
}