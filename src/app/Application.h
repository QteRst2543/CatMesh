#pragma once

#include "Platform/Window.h"
#include "app/SceneDocument.h"
#include "ui/UI.h"
#include "ui/Localization.h"
#include "core/Camera.h"
#include "core/Grid.h"
#include <string>

enum class ToolMode {
    Select,
    Move,
    Rotate,
    Extrude,
    Split
};

class Application {
public:
    Application();
    ~Application();

    void Run();

    // Геттеры
    Mesh* GetSelectedMesh() const { return scene.GetSelectedMesh(); }
    const auto& GetMeshes() const { return scene.GetMeshes(); }
    Camera& GetCamera() { return camera; }
    LightState& GetLight() { return scene.GetLight(); }
    const LightState& GetLight() const { return scene.GetLight(); }
    Language GetLanguage() const { return language; }
    SelectedEntityType GetSelectedEntityType() const { return scene.GetSelectedEntityType(); }
    bool IsLightSelected() const { return scene.IsLightSelected(); }
    std::shared_ptr<Mesh> GetSelectedMeshHandle() const;

    void SetLanguage(Language newLanguage) { language = newLanguage; }
    void SetShowLightRays(bool show) { showLightRays = show; }
    bool GetShowLightRays() const { return showLightRays; }
    void SetToolMode(ToolMode mode) { toolMode = mode; }
    ToolMode GetToolMode() const { return toolMode; }
    void SetRotationAxis(int axis) { rotationAxis = axis; }
    int GetRotationAxis() const { return rotationAxis; }
    void SelectMesh(Mesh* mesh);
    void SelectLight();
    void AddDefaultCube();
    void AddLightSource();
    void DeleteSelectedMesh();
    void DeleteSelectedObject();
    void ExecuteCommand(std::unique_ptr<Command> cmd);
    void Undo();
    void Redo();

    // Для команд
    Mesh* AddMesh(const std::shared_ptr<Mesh>& mesh);
    void RemoveMesh(Mesh* mesh);

    // Файловые операции
    void OpenFile();
    void ImportImage();
    void SaveFile();
    void ExportSTL(const std::string& path);

private:
    void InitShadowMapping();
    Window window;
    UIManager ui;
    Camera camera;
    SceneDocument scene;
    Language language = Language::English;
    bool showLightRays = true;
    ToolMode toolMode = ToolMode::Select;
    int rotationAxis = 1;

    float lastLeftMouseX, lastLeftMouseY;
    float lastRightMouseX, lastRightMouseY;
    bool leftMousePressed;
    bool rightMousePressed;

    // Shadow mapping
    unsigned int shadowFBO = 0;
    unsigned int shadowMap = 0;
    unsigned int shadowShaderProgram = 0;
    glm::mat4 lightSpaceMatrix;
    double dragStartMouseX, dragStartMouseY;
    bool isDraggingElement;
    glm::vec3 objectDragOffset;
    bool hasObjectDragOffset;
    glm::vec3 lastDragIntersection;
    bool hasLastDragIntersection;
    glm::vec3 dragPlanePoint;
    glm::vec3 dragPlaneNormal;
    bool hasDragPlane;

    // Состояние текущей операции Extrude
    bool extrudeOperationActive = false;
    bool extrudePerformed = false;
    bool extrudeTranslateMode = false;
    int extrudeFaceIndex = -1;
    float extrudeDistance = 0.0f;
    void HandleInput();
    void ClearHistory();
    void BeginMeshEditTracking(const std::shared_ptr<Mesh>& mesh);
    void CommitMeshEditTracking();
    void ResetMeshEditTracking();
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
