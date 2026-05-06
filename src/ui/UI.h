#pragma once

#include "Themes.h"
#include "Localization.h"

class Application;
class Window;

class UIManager {
public:
    UIManager(Window& window, Application* app);
    ~UIManager();
    void NewFrame();
    void Render();
    void RenderDrawData();
    void SetTheme(Themes theme);

private:
    Window& window;
    Application* app;
    Themes currentTheme;
    bool initialized = false;
    bool showToolsWindow = true;
    bool showObjectParamsWindow = true;
    bool showControlsWindow = true;
    bool showAddObjectModal = false;
    bool showLightRays = true;
    float moveStep = 0.25f;
    float extrudeStep = 0.1f;

    void DrawMainMenuBar();
    void DrawToolsWindow();
    void DrawControlsWindow();
    void DrawObjectParams();
    void DrawAddObjectModal();
};
