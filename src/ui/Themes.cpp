#include"Themes.h"
#include"imgui.h"

static void ApplyPS1Theme() {
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 0.0f;
	style.FrameRounding = 0.0f;
	style.GrabRounding = 0.0f;
	style.ScrollbarRounding = 0.0f;
	ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.30f, 0.30f, 0.70f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.45f, 0.85f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.55f, 0.55f, 0.95f, 1.0f);

    colors[ImGuiCol_Button] = ImVec4(0.30f, 0.30f, 0.70f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.45f, 0.45f, 0.85f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.55f, 0.55f, 0.95f, 1.0f);

    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.25f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.35f, 0.35f, 0.45f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.45f, 0.45f, 0.60f, 1.0f);

    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.15f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.20f, 0.35f, 1.0f);

    colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.8f, 0.1f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.9f, 0.2f, 1.0f);
}

void ApplyTheme(Themes theme) {
    switch (theme)
    {
    case Themes::Dark:
        ImGui::StyleColorsDark();
        break;
    case Themes::Light:
        ImGui::StyleColorsLight();
        break;
    case Themes::Classic:
        ImGui::StyleColorsClassic();
        break;
    case Themes::PS1:
        ApplyPS1Theme();
        break;
    }
}