#include "Window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

void Window::ErrorCallback(int error, const char* description) {
	std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}
Window::Window(int width, int height, const char* title)
	: width(width), height(height) {
	glfwSetErrorCallback(ErrorCallback);
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl; 
		window = nullptr;
		return;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); //висинк

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate(); 
		window = nullptr;
		return;
	}
	glEnable(GL_DEPTH_TEST);
}
Window::~Window() {
	if (window) {
		glfwDestroyWindow(window);
	}
	glfwTerminate();
}

bool Window::ShouldClose() const {
	return window ? glfwWindowShouldClose(window) : true;
}

void Window::PollEvents() {
	glfwPollEvents();
}

void Window::SwapBuffers() {
	glfwSwapBuffers(window);
}

int Window::GetWidth() const {
	return width;
}

int Window::GetHeight() const {
	return height;
}

GLFWwindow* Window::GetNativeWindow() const {
	return window;
}

bool Window::IsValid() const {
	return window != nullptr;
}