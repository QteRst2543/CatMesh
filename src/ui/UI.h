#pragma once
#include "Themes.h"

class Window; 

class UIManager {
public:
	UIManager(Window& window);
	~UIManager();

	void NewFrame();
	void Render();
	void RenderDrawData();
	void SetTheme(Themes theme);

private:
	Window& window;
	Themes currentTheme;

	void DrawMainMenuBar();
	void DrawToolsWindow();
	void DrawObjectParams();
};