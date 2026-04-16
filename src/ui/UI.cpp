#include "UI.h"
#include "Localization.h"
#include "app/Application.h" 
#include "Platform/Window.h"
#include <glm/glm.hpp>
#include "imgui.h"
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
    auto& loc = Localization::Instance();
    
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu(loc.Get("file"))) {
            if (ImGui::MenuItem(loc.Get("open_import"), "Ctrl+O")) {
                if (app) app->OpenFile();
            }
            if (ImGui::MenuItem(loc.Get("import_image"), "Ctrl+I")) {
                if (app) app->ImportImage();
            }
            if (ImGui::MenuItem(loc.Get("save_scene"), "Ctrl+S")) {
                if (app) app->SaveFile();
            }
            if (ImGui::MenuItem(loc.Get("export_stl"))) {
                if (app) app->ExportSTL("");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(loc.Get("edit"))) {
            if (ImGui::MenuItem(loc.Get("undo"), "Ctrl+Z")) {
                if (app) app->Undo();
            }
            if (ImGui::MenuItem(loc.Get("redo"), "Ctrl+Y")) {
                if (app) app->Redo();
            }
            if (ImGui::BeginMenu(loc.Get("theme"))) {
                if (ImGui::MenuItem(loc.Get("dark"))) SetTheme(Themes::Dark);
                if (ImGui::MenuItem(loc.Get("light"))) SetTheme(Themes::Light);
                if (ImGui::MenuItem(loc.Get("classic"))) SetTheme(Themes::Classic);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(loc.Get("language"))) {
            if (ImGui::MenuItem(loc.Get("english"))) {
                loc.SetLanguage(Language::English);
                if (app) app->SetLanguage(Language::English);
            }
            if (ImGui::MenuItem(loc.Get("russian"))) {
                loc.SetLanguage(Language::Russian);
                if (app) app->SetLanguage(Language::Russian);
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void UIManager::DrawToolsWindow() {
    auto& loc = Localization::Instance();
    ImGui::SetNextWindowPos(ImVec2(10, 40), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(340, 0), ImVec2(340, 600));
    ImGui::Begin(loc.Get("tools"), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);

    if (ImGui::Button(loc.Get("object_mode"))) {
        if (app && app->GetSelectedMesh())
            app->GetSelectedMesh()->SetEditMode(Mesh::EditMode::Object);
    }
    ImGui::SameLine();
    if (ImGui::Button(loc.Get("face_mode"))) {
        if (app && app->GetSelectedMesh())
            app->GetSelectedMesh()->SetEditMode(Mesh::EditMode::Face);
    }

    ImGui::Separator();
    ToolMode currentTool = app ? app->GetToolMode() : ToolMode::Select;
    ImGui::Text("%s:", loc.Get("tool"));
    const char* toolNames[] = {
        loc.Get("select_tool"),
        loc.Get("move_tool"),
        loc.Get("rotate_tool"),
        loc.Get("extrude_tool"),
        loc.Get("split_tool")
    };
    for (int toolIndex = 0; toolIndex < 5; ++toolIndex) {
        if (ImGui::RadioButton(toolNames[toolIndex], currentTool == static_cast<ToolMode>(toolIndex))) {
            if (app) app->SetToolMode(static_cast<ToolMode>(toolIndex));
            currentTool = static_cast<ToolMode>(toolIndex);
        }
        if (toolIndex % 3 != 2 && toolIndex != 4) {
            ImGui::SameLine();
        }
    }

    if (currentTool == ToolMode::Rotate) {
        ImGui::Text(loc.Get("axis_only"));
        int currentRotationAxis = app ? app->GetRotationAxis() : 1;
        if (ImGui::RadioButton("X", &currentRotationAxis, 0) && app) app->SetRotationAxis(currentRotationAxis);
        ImGui::SameLine();
        if (ImGui::RadioButton("Y", &currentRotationAxis, 1) && app) app->SetRotationAxis(currentRotationAxis);
        ImGui::SameLine();
        if (ImGui::RadioButton("Z", &currentRotationAxis, 2) && app) app->SetRotationAxis(currentRotationAxis);
    }

    if (currentTool == ToolMode::Extrude) {
        ImGui::TextWrapped(loc.Get("extrude_hint"));
    }
    else if (currentTool == ToolMode::Split) {
        ImGui::TextWrapped(loc.Get("split_hint"));
    }

    ImGui::Separator();
    ImGui::TextWrapped(loc.Get("lmb_info"));
    ImGui::TextWrapped(loc.Get("rmb_info"));
    ImGui::TextWrapped(loc.Get("wasd_info"));

    if (app && ImGui::CollapsingHeader(loc.Get("light_props"), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& light = app->GetLight();

        ImGui::DragFloat3(loc.Get("light_position"), glm::value_ptr(light.position), 0.1f);
        if (ImGui::DragFloat3(loc.Get("light_direction"), glm::value_ptr(light.direction), 0.05f)) {
            if (glm::length(light.direction) > 0.0001f) {
                light.direction = glm::normalize(light.direction);
            }
        }

        ImGui::ColorEdit3(loc.Get("light_color"), glm::value_ptr(light.color));
        ImGui::SliderFloat(loc.Get("light_intensity"), &light.intensity, 0.1f, 12.0f);
        ImGui::SliderFloat(loc.Get("ambient"), &light.ambientStrength, 0.05f, 0.6f);
        ImGui::SliderFloat(loc.Get("inner_cone"), &light.innerCutoffDegrees, 5.0f, 45.0f);
        ImGui::SliderFloat(loc.Get("outer_cone"), &light.outerCutoffDegrees, 8.0f, 60.0f);

        if (light.outerCutoffDegrees < light.innerCutoffDegrees + 1.0f) {
            light.outerCutoffDegrees = light.innerCutoffDegrees + 1.0f;
        }

        bool currentShowLightRays = app->GetShowLightRays();
        if (ImGui::Checkbox(loc.Get("show_light_rays"), &currentShowLightRays)) {
            app->SetShowLightRays(currentShowLightRays);
        }
    }

    ImGui::End();
}

void UIManager::DrawObjectParams() {
    if (!app) return;

    auto& loc = Localization::Instance();
    ImGui::SetNextWindowPos(ImVec2(10, 260), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(340, 0), ImVec2(340, 700));
    ImGui::Begin(loc.Get("object_parameters"), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginListBox(loc.Get("objects"), ImVec2(-1, 140))) {
        const auto& meshes = app->GetMeshes();
        for (size_t i = 0; i < meshes.size(); i++) {
            std::string label = loc.Get("cube");
            label += " ";
            label += std::to_string(i + 1);
            bool isSelected = (app->GetSelectedMesh() == meshes[i].get() && app->GetSelectedEntityType() == SelectedEntityType::Mesh);
            if (ImGui::Selectable(label.c_str(), isSelected)) {
                app->SelectMesh(meshes[i].get());
            }
        }

        if (app->GetLight().enabled) {
            bool lightSelected = app->GetSelectedEntityType() == SelectedEntityType::Light;
            if (ImGui::Selectable(loc.Get("light_source"), lightSelected)) {
                app->SelectLight();
            }
        }

        ImGui::EndListBox();
    }

    if (ImGui::Button(loc.Get("add"))) {
        showAddObjectModal = true;
        ImGui::OpenPopup(loc.Get("add_object"));
    }
    ImGui::SameLine();
    if (ImGui::Button(loc.Get("delete_selected"))) {
        app->DeleteSelectedObject();
    }

    if (app->GetSelectedEntityType() == SelectedEntityType::Mesh && app->GetSelectedMesh()) {
        auto* selected = app->GetSelectedMesh();
        glm::vec3 pos = selected->GetPosition();
        glm::vec3 col = selected->GetColor();
        const auto selectedHandle = app->GetSelectedMeshHandle();

        if (selectedHandle && ImGui::DragFloat3(loc.Get("position"), glm::value_ptr(pos), 0.1f)) {
            app->ExecuteCommand(std::make_unique<MoveCommand>(selectedHandle, pos));
        }
        if (selectedHandle && ImGui::ColorEdit3(loc.Get("color"), glm::value_ptr(col))) {
            app->ExecuteCommand(std::make_unique<ColorCommand>(selectedHandle, col));
        }
        bool wireframe = selected->GetShowWireframe();
        if (ImGui::Checkbox(loc.Get("wireframe"), &wireframe)) {
            selected->SetShowWireframe(wireframe);
        }

        ImGui::Text("%s: ", loc.Get("edit_mode"));
        ImGui::SameLine();
        switch (selected->GetEditMode()) {
        case Mesh::EditMode::Object: ImGui::Text(loc.Get("object")); break;
        case Mesh::EditMode::Vertex: ImGui::Text(loc.Get("vertex")); break;
        case Mesh::EditMode::Edge: ImGui::Text(loc.Get("edge")); break;
        case Mesh::EditMode::Face: ImGui::Text(loc.Get("face")); break;
        }
    }
    else if (app->GetSelectedEntityType() == SelectedEntityType::Light && app->GetLight().enabled) {
        auto& light = app->GetLight();
        ImGui::DragFloat3(loc.Get("light_position"), glm::value_ptr(light.position), 0.1f);
        if (ImGui::DragFloat3(loc.Get("light_direction"), glm::value_ptr(light.direction), 0.05f)) {
            if (glm::length(light.direction) > 0.0001f) {
                light.direction = glm::normalize(light.direction);
            }
        }
        ImGui::ColorEdit3(loc.Get("light_color"), glm::value_ptr(light.color));
        ImGui::SliderFloat(loc.Get("light_intensity"), &light.intensity, 0.1f, 12.0f);
        ImGui::SliderFloat(loc.Get("ambient"), &light.ambientStrength, 0.05f, 0.6f);
        ImGui::SliderFloat(loc.Get("inner_cone"), &light.innerCutoffDegrees, 5.0f, 45.0f);
        ImGui::SliderFloat(loc.Get("outer_cone"), &light.outerCutoffDegrees, 8.0f, 60.0f);

        if (light.outerCutoffDegrees < light.innerCutoffDegrees + 1.0f) {
            light.outerCutoffDegrees = light.innerCutoffDegrees + 1.0f;
        }
    }
    else {
        ImGui::Text(loc.Get("no_object_selected"));
    }

    ImGui::End();

    DrawAddObjectModal();
}

void UIManager::DrawAddObjectModal() {
    auto& loc = Localization::Instance();
    
    if (showAddObjectModal) {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::OpenPopup(loc.Get("add_object"));
        if (ImGui::BeginPopupModal(loc.Get("add_object"), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (ImGui::BeginTabBar("ObjectCategories")) {
                if (ImGui::BeginTabItem(loc.Get("objects"))) {
                    if (ImGui::Selectable(loc.Get("cube"))) {
                        if (app) app->ExecuteCommand(std::make_unique<AddCubeCommand>(app));
                        showAddObjectModal = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem(loc.Get("utilities"))) {
                    if (ImGui::Selectable(loc.Get("light_source"))) {
                        if (app) app->AddLightSource();
                        showAddObjectModal = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndTabItem();
                }
                
                ImGui::EndTabBar();
            }
            ImGui::EndPopup();
        }
        if (!ImGui::IsPopupOpen(loc.Get("add_object"))) {
            showAddObjectModal = false;
        }
    }
}
