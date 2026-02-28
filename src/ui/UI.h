#pragma once

#include "Themes.h"

class Application;
class Window; 

class UIManager {
public:
	UIManager(Window& window, Application* app); // указатель на приложение
	~UIManager();
	void NewFrame();
	void Render();
	void RenderDrawData();
	void SetTheme(Themes theme);

private:
	Window& window;
	Application* app;
	Themes currentTheme;

	void DrawMainMenuBar();
	void DrawToolsWindow();
	void DrawObjectParams();
};