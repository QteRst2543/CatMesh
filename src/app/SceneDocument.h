#pragma once

#include "app/SceneTypes.h"
#include "core/Command.h"
#include "core/Mesh.h"
#include <memory>
#include <optional>
#include <stack>
#include <vector>

class SceneDocument {
public:
    Mesh* GetSelectedMesh() const { return selectedMesh; }
    const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const { return meshes; }
    std::vector<std::shared_ptr<Mesh>>& AccessMeshes() { return meshes; }
    LightState& GetLight() { return light; }
    const LightState& GetLight() const { return light; }
    LightState& AccessLight() { return light; }
    SelectedEntityType GetSelectedEntityType() const { return selectedEntityType; }
    SelectedEntityType& AccessSelectedEntityType() { return selectedEntityType; }
    Mesh*& AccessSelectedMesh() { return selectedMesh; }
    bool IsLightSelected() const { return selectedEntityType == SelectedEntityType::Light && light.enabled; }
    std::shared_ptr<Mesh> GetSelectedMeshHandle() const;
    glm::vec3 GetSceneCenter() const;

    void SelectMesh(Mesh* mesh);
    void SelectLight();
    void ClearSelection();
    void ReplaceScene(std::vector<std::shared_ptr<Mesh>> newMeshes);

    void AddDefaultCube();
    void AddLightSource();
    void DeleteSelectedMesh();
    void DeleteSelectedObject();

    void ExecuteCommand(std::unique_ptr<Command> cmd);
    void Undo();
    void Redo();

    Mesh* AddMesh(const std::shared_ptr<Mesh>& mesh);
    void RemoveMesh(Mesh* mesh);

    void ClearHistory();
    void BeginMeshEditTracking(const std::shared_ptr<Mesh>& mesh);
    void CommitMeshEditTracking();
    void ResetMeshEditTracking();

private:
    std::vector<std::shared_ptr<Mesh>> meshes;
    Mesh* selectedMesh = nullptr;
    SelectedEntityType selectedEntityType = SelectedEntityType::None;
    std::stack<std::unique_ptr<Command>> undoStack;
    std::stack<std::unique_ptr<Command>> redoStack;
    LightState light;
    std::shared_ptr<Mesh> activeMeshEditTarget;
    std::optional<Mesh::State> activeMeshEditBeforeState;
};
