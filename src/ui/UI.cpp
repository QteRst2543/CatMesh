#include "UI.h"
#include "app/Application.h" 
#include "Platform/Window.h"
#include <glm/glm.hpp>
#include "imgui.h"
#include "app/Application.h"
#include "core/Mesh.h"
#include <glm/gtc/type_ptr.hpp>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "core/Command.h"
#include <string>
#include <filesystem>

UIManager::UIManager(Window& win, Application* app)
    : window(win), app(app), currentTheme(Themes::Classic)
{
    if (!window.IsValid()) {
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ApplyTheme(currentTheme);

    ImGui_ImplGlfw_InitForOpenGL(window.GetNativeWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 460");
    initialized = true;
}

UIManager::~UIManager() {
    if (!initialized) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::NewFrame() {
    if (!initialized) {
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::Render() {
    if (!initialized) {
        return;
    }

    DrawMainMenuBar();
    DrawToolsWindow();
    DrawObjectParams();
    ImGui::Render();
}

void UIManager::RenderDrawData() {
    if (!initialized) {
        return;
    }

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::SetTheme(Themes theme) {
    currentTheme = theme;
    ApplyTheme(theme);
}

void UIManager::DrawMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open / Import", "Ctrl+O")) {
                if (app) app->OpenFile();
            }
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                if (app) app->SaveFile();
            }
            if (ImGui::MenuItem("Export STL")) {
                if (app) app->ExportSTL("");
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
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void UIManager::DrawToolsWindow() {
    ImGui::Begin("Tools");

    if (ImGui::Button("Object Mode")) {
        if (app && app->GetSelectedMesh())
            app->GetSelectedMesh()->SetEditMode(Mesh::EditMode::Object);
    }
    if (ImGui::Button("Face Mode")) {
        if (app && app->GetSelectedMesh())
            app->GetSelectedMesh()->SetEditMode(Mesh::EditMode::Face);
    }

    ImGui::Separator();
    ImGui::TextWrapped("LMB selects objects in viewport and drags them.");
    ImGui::TextWrapped("RMB orbits around the scene or the selected object.");
    ImGui::TextWrapped("Mouse wheel zooms. WASD moves the camera target.");

    if (app && ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& light = app->GetLight();

        ImGui::DragFloat3("Light Position", glm::value_ptr(light.position), 0.1f);
        if (ImGui::DragFloat3("Light Direction", glm::value_ptr(light.direction), 0.05f)) {
            if (glm::length(light.direction) > 0.0001f) {
                light.direction = glm::normalize(light.direction);
            }
        }

        ImGui::ColorEdit3("Light Color", glm::value_ptr(light.color));
        ImGui::SliderFloat("Light Intensity", &light.intensity, 0.1f, 12.0f);
        ImGui::SliderFloat("Ambient", &light.ambientStrength, 0.05f, 0.6f);
        ImGui::SliderFloat("Inner Cone", &light.innerCutoffDegrees, 5.0f, 45.0f);
        ImGui::SliderFloat("Outer Cone", &light.outerCutoffDegrees, 8.0f, 60.0f);

        if (light.outerCutoffDegrees < light.innerCutoffDegrees + 1.0f) {
            light.outerCutoffDegrees = light.innerCutoffDegrees + 1.0f;
        }
    }

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
        const auto selectedHandle = app->GetSelectedMeshHandle();

        if (selectedHandle && ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.1f)) {
            app->ExecuteCommand(std::make_unique<MoveCommand>(selectedHandle, pos));
        }
        if (selectedHandle && ImGui::ColorEdit3("Color", glm::value_ptr(col))) {
            app->ExecuteCommand(std::make_unique<ColorCommand>(selectedHandle, col));
        }

        // Отображаем текущий режим редактирования
        ImGui::Text("Edit Mode: ");
        ImGui::SameLine();
        switch (selected->GetEditMode()) {
        case Mesh::EditMode::Object: ImGui::Text("Object"); break;
        case Mesh::EditMode::Vertex: ImGui::Text("Vertex"); break;
        case Mesh::EditMode::Edge: ImGui::Text("Edge"); break;
        case Mesh::EditMode::Face: ImGui::Text("Face"); break;
        }
    }
    else {
        ImGui::Text("No object selected");
    }

    ImGui::End();
}
