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
#include <string>

enum class Language {
    English,
    Russian
};

enum class SelectedEntityType {
    None,
    Mesh,
    Light
};

struct LightState {
    bool enabled = true;
    glm::vec3 position = glm::vec3(3.0f, 4.0f, 3.0f);
    glm::vec3 direction = glm::normalize(glm::vec3(-1.0f, -1.2f, -0.8f));
    glm::vec3 color = glm::vec3(1.0f, 0.96f, 0.9f);
    float intensity = 6.0f;
    float ambientStrength = 0.28f;
    float innerCutoffDegrees = 18.0f;
    float outerCutoffDegrees = 30.0f;
};

class Application {
public:
    Application();
    ~Application();

    void Run();

    // Геттеры
    Mesh* GetSelectedMesh() const { return selectedMesh; }
    const auto& GetMeshes() const { return meshes; }
    Camera& GetCamera() { return camera; }
    LightState& GetLight() { return light; }
    const LightState& GetLight() const { return light; }
    Language GetLanguage() const { return language; }
    SelectedEntityType GetSelectedEntityType() const { return selectedEntityType; }
    bool IsLightSelected() const { return selectedEntityType == SelectedEntityType::Light && light.enabled; }
    std::shared_ptr<Mesh> GetSelectedMeshHandle() const;

    void SetLanguage(Language newLanguage) { language = newLanguage; }
    void SelectMesh(Mesh* mesh);
    void SelectLight();
    void AddDefaultCube();
    void AddLightSource();
    void DeleteSelectedMesh();
    void ExecuteCommand(std::unique_ptr<Command> cmd);
    void Undo();
    void Redo();

    // Для команд
    Mesh* AddMesh(const std::shared_ptr<Mesh>& mesh);
    void RemoveMesh(Mesh* mesh);

    // Файловые операции
    void OpenFile();
    void SaveFile();
    void ExportSTL(const std::string& path);

private:
    Window window;
    UIManager ui;
    Camera camera;
    std::vector<std::shared_ptr<Mesh>> meshes;
    Mesh* selectedMesh = nullptr;
    SelectedEntityType selectedEntityType = SelectedEntityType::None;
    std::stack<std::unique_ptr<Command>> undoStack;
    std::stack<std::unique_ptr<Command>> redoStack;
    LightState light;
    Language language = Language::English;

    float lastLeftMouseX, lastLeftMouseY;
    float lastRightMouseX, lastRightMouseY;
    bool leftMousePressed;
    bool rightMousePressed;
    double dragStartMouseX, dragStartMouseY;
    bool isDraggingElement;
    glm::vec3 objectDragOffset;
    bool hasObjectDragOffset;
    glm::vec3 lastDragIntersection;
    bool hasLastDragIntersection;
    glm::vec3 dragPlanePoint;
    glm::vec3 dragPlaneNormal;
    bool hasDragPlane;

    void HandleInput();
    void ClearHistory();
    bool LoadScene(const std::string& path);
    bool SaveSceneToPath(const std::string& path) const;
    glm::vec3 GetSceneCenter() const;
    glm::vec3 GetOrbitFocus() const;
    glm::vec3 GetSelectionAnchor() const;
    void HandleCameraKeyboard(GLFWwindow* glfwWindow, float deltaTime, bool wantsKeyboard);
    glm::vec3 GetRayFromMouse(double mouseX, double mouseY, int screenWidth, int screenHeight);
    glm::vec3 GetPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
        const glm::vec3& planePoint, const glm::vec3& planeNormal);
};
