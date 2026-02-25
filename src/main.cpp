#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "app/Application.h"
#include "ui/Themes.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <iostream>

int main() {
    Application app;
    app.Run();
    return 0;
}
