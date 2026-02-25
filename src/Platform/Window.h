#pragma once
#include <iostream>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <string>

class Window {
public:
	Window(int wight, int height, const char* title);
	~Window();

	bool ShouldClose() const;
	void PollEvents();
	void SwapBuffers();
	int GetWidth() const;
	int GetHeight() const;
	GLFWwindow* GetNativeWindow() const;
	bool IsValid() const; // проверяем открытое окно 

private:
	GLFWwindow* window;
	int width, height;

	static void ErrorCallback(int error, const char* description);
};