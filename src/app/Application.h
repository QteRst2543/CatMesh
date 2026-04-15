#pragma once

#include "Platform/Window.h"
#include "ui/UI.h"
#include "core/Camera.h"
#include "core/Mesh.h"
#include "core/Command.h"
#include "core/Grid.h"
#include <vector>
#include <memory>
#include <stack>

class Application {
public:
    Application();
    ~Application();

    void Run();

    // Геттеры
    Mesh* GetSelectedMesh() const { return selectedMesh; }
    const auto& GetMeshes() const { return meshes; }
    Camera& GetCamera() { return camera; }

    void SelectMesh(Mesh* mesh) { selectedMesh = mesh; }
    void AddDefaultCube();
    void DeleteSelectedMesh();
    void ExecuteCommand(std::unique_ptr<Command> cmd);
    void Undo();
    void Redo();

    // Для команд
    Mesh* AddMesh(std::unique_ptr<Mesh> mesh);
    void RemoveMesh(Mesh* mesh);

    // Файловые операции
    void OpenFile();
    void SaveFile();
    void ExportSTL(const std::string& path);

private:
    Window window;
    UIManager ui;
    Camera camera;
    std::vector<std::unique_ptr<Mesh>> meshes;
    Mesh* selectedMesh = nullptr;
    std::stack<std::unique_ptr<Command>> undoStack;
    std::stack<std::unique_ptr<Command>> redoStack;

    float lastMouseX, lastMouseY;
    bool mousePressed;
    double dragStartMouseX, dragStartMouseY;
    bool isDraggingElement;

    void HandleInput();
    glm::vec3 GetRayFromMouse(double mouseX, double mouseY, int screenWidth, int screenHeight);
    glm::vec3 GetPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
        const glm::vec3& planePoint, const glm::vec3& planeNormal);
};