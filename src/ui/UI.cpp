#include "UI.h"
#include "Platform/Window.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <string>

UIManager::UIManager(Window& win)
    : window(win), currentTheme(Themes::Classic)
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
    ImGui::Render(); // подготавливает данные для рендера
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
            ImGui::MenuItem("Open");
            ImGui::MenuItem("Save");
            ImGui::MenuItem("Export STL");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Undo");
            ImGui::MenuItem("Redo");
            if (ImGui::BeginMenu("Theme")) {
                if (ImGui::Button("Dark"))
                    SetTheme(Themes::Dark);
                if (ImGui::Button("Light"))
                    SetTheme(Themes::Light);
                if (ImGui::Button("Classic"))
                    SetTheme(Themes::Classic);
                if (ImGui::Button("PS1"))
                    SetTheme(Themes::PS1);
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
    ImGui::Begin("Object parameters");
    ImGui::Text("No object selected");
    ImGui::End();
}