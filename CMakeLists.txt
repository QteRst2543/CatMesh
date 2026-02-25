cmake_minimum_required(VERSION 3.16)
project(CatMesh_Dev)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(extern/glfw)

# Библиотека glad
add_library(glad STATIC
    extern/glad/src/glad.c
 #"src/core/Camera.h" "src/core/Camera.cpp" "src/core/Mesh.h" "src/core/Mesh.cpp" "src/core/Shader.h" "src/core/Shader.cpp")

target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/include
)

# Библиотека imgui
add_library(imgui STATIC
    extern/imgui/imgui.cpp
    extern/imgui/imgui_draw.cpp
    extern/imgui/imgui_tables.cpp
    extern/imgui/imgui_widgets.cpp
    extern/imgui/backends/imgui_impl_glfw.cpp
    extern/imgui/backends/imgui_impl_opengl3.cpp
 #"src/core/Camera.h" "src/core/Camera.cpp" "src/core/Mesh.h" "src/core/Mesh.cpp" "src/core/Shader.h" "src/core/Shader.cpp")

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/glfw/include
)

# Исполняемый файл
add_executable(CatMesh_Dev
    src/main.cpp
    src/app/Application.cpp
    src/Platform/Window.cpp
    src/ui/UI.cpp
    src/ui/Themes.cpp
 "src/core/Camera.h" "src/core/Camera.cpp" "src/core/Mesh.h" "src/core/Mesh.cpp" "src/core/Shader.h" "src/core/Shader.cpp")

# Все include директории через PUBLIC
target_include_directories(CatMesh_Dev PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/glm
)

target_link_libraries(CatMesh_Dev
    glfw
    glad
    imgui
    opengl32
)