#pragma once

#include "Platform/Window.h"
#include "ui/UI.h"
#include "core/Camera.h"
#include "core/Mesh.h"

class Application {
public:
	Application();
	~Application();
	void Run();

private:
	Window window;
	UIManager ui;
	Camera camera;
	Mesh* mesh;

	float lastMouseX, lastMouseY;
	bool mousePressed;

	void HandleInput();
};