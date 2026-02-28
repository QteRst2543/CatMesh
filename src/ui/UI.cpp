#include "UI.h"
#include "app/Application.h" 
#include "Platform/Window.h"
#include <glm/glm.hpp>
#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "core/Command.h"
#include <string>

UIManager::UIManager(Window& win, Application* app)
    : window(win), app(app), currentTheme(Themes::Classic)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ApplyTheme(currentTheme);

    ImGui_ImplGlfw_InitForOpenGL(window.GetNativeWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 460");
}

UIManager::~UIManager() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::NewFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::Render() {
    DrawMainMenuBar();
    DrawToolsWindow();
    DrawObjectParams();
    ImGui::Render();
}

void UIManager::RenderDrawData() {
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::SetTheme(Themes theme) {
    currentTheme = theme;
    ApplyTheme(theme);
}

void UIManager::DrawMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                if (app) app->OpenFile();
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                if (app) app->SaveFile();
            }
            if (ImGui::MenuItem("Export STL")) {
                if (app) app->ExportSTL();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                if (app) app->Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                if (app) app->Redo();
            }
            if (ImGui::BeginMenu("Theme")) {
                if (ImGui::MenuItem("Dark")) SetTheme(Themes::Dark);
                if (ImGui::MenuItem("Light")) SetTheme(Themes::Light);
                if (ImGui::MenuItem("Classic")) SetTheme(Themes::Classic);
                if (ImGui::MenuItem("PS1")) SetTheme(Themes::PS1);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void UIManager::DrawToolsWindow() {
    ImGui::Begin("Tools");
    ImGui::Button("Vertex select");
    ImGui::Button("Move vertex");
    ImGui::End();
}

void UIManager::DrawObjectParams() {
    if (!app) return;

    ImGui::Begin("Object parameters");

    // Список объектов с возможностью выбора
    if (ImGui::BeginListBox("Objects", ImVec2(-1, 100))) {
        const auto& meshes = app->GetMeshes();
        for (size_t i = 0; i < meshes.size(); i++) {
            std::string label = "Cube " + std::to_string(i + 1);
            bool isSelected = (app->GetSelectedMesh() == meshes[i].get());

            if (ImGui::Selectable(label.c_str(), isSelected)) {
                app->SelectMesh(meshes[i].get());
            }
        }
        ImGui::EndListBox();
    }

    if (ImGui::Button("Add Cube")) {
        app->ExecuteCommand(std::make_unique<AddCubeCommand>(app));
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete Selected")) {
        app->DeleteSelectedMesh();
    }

    // Редактирование свойств выбранного объекта
    if (auto* selected = app->GetSelectedMesh()) {
        glm::vec3 pos = selected->GetPosition();
        glm::vec3 col = selected->GetColor();

        if (ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.1f)) {
            app->ExecuteCommand(std::make_unique<MoveCommand>(selected, pos));
        }
        if (ImGui::ColorEdit3("Color", glm::value_ptr(col))) {
            app->ExecuteCommand(std::make_unique<ColorCommand>(selected, col));
        }
    }
    else {
        ImGui::Text("No object selected");
    }

    ImGui::End();
}