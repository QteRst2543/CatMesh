#include "app/SceneDocument.h"
#include <algorithm>
#include <iostream>

std::shared_ptr<Mesh> SceneDocument::GetSelectedMeshHandle() const {
    auto it = std::find_if(meshes.begin(), meshes.end(),
        [this](const auto& mesh) { return mesh.get() == selectedMesh; });
    return it != meshes.end() ? *it : nullptr;
}

glm::vec3 SceneDocument::GetSceneCenter() const {
    if (meshes.empty()) {
        return glm::vec3(0.0f);
    }

    glm::vec3 center(0.0f);
    for (const auto& mesh : meshes) {
        center += mesh->GetPosition();
    }
    return center / static_cast<float>(meshes.size());
}

void SceneDocument::SelectMesh(Mesh* mesh) {
    selectedMesh = mesh;
    selectedEntityType = mesh ? SelectedEntityType::Mesh : SelectedEntityType::None;
}

void SceneDocument::SelectLight() {
    selectedMesh = nullptr;
    selectedEntityType = light.enabled ? SelectedEntityType::Light : SelectedEntityType::None;
}

void SceneDocument::ClearSelection() {
    selectedMesh = nullptr;
    selectedEntityType = SelectedEntityType::None;
}

void SceneDocument::ReplaceScene(std::vector<std::shared_ptr<Mesh>> newMeshes) {
    meshes = std::move(newMeshes);
    selectedMesh = meshes.empty() ? nullptr : meshes.front().get();
    selectedEntityType = selectedMesh ? SelectedEntityType::Mesh : SelectedEntityType::None;
    ClearHistory();
}

void SceneDocument::ClearHistory() {
    while (!undoStack.empty()) {
        undoStack.pop();
    }
    while (!redoStack.empty()) {
        redoStack.pop();
    }
    ResetMeshEditTracking();
}

void SceneDocument::BeginMeshEditTracking(const std::shared_ptr<Mesh>& mesh) {
    if (!mesh) {
        ResetMeshEditTracking();
        return;
    }

    activeMeshEditTarget = mesh;
    activeMeshEditBeforeState = mesh->CaptureState();
}

void SceneDocument::CommitMeshEditTracking() {
    if (!activeMeshEditTarget || !activeMeshEditBeforeState.has_value()) {
        ResetMeshEditTracking();
        return;
    }

    if (!activeMeshEditTarget->MatchesState(*activeMeshEditBeforeState)) {
        ExecuteCommand(std::make_unique<MeshStateCommand>(
            activeMeshEditTarget,
            *activeMeshEditBeforeState,
            activeMeshEditTarget->CaptureState()));
    }

    ResetMeshEditTracking();
}

void SceneDocument::ResetMeshEditTracking() {
    activeMeshEditTarget.reset();
    activeMeshEditBeforeState.reset();
}

void SceneDocument::AddDefaultCube() {
    auto mesh = std::make_shared<Mesh>();
    if (!mesh->LoadFromFile("")) {
        std::cerr << "Failed to create default cube." << std::endl;
        return;
    }

    mesh->SetPosition(glm::vec3(0.0f));
    mesh->SetColor(glm::vec3(0.5f, 0.5f, 0.5f));
    mesh->SetEditMode(Mesh::EditMode::Object);
    meshes.push_back(mesh);
    selectedMesh = mesh.get();
    selectedEntityType = SelectedEntityType::Mesh;
}

void SceneDocument::DeleteSelectedMesh() {
    if (!selectedMesh) {
        return;
    }

    auto it = std::find_if(meshes.begin(), meshes.end(),
        [this](const auto& ptr) { return ptr.get() == selectedMesh; });
    if (it == meshes.end()) {
        return;
    }

    ClearHistory();
    meshes.erase(it);
    selectedMesh = nullptr;
    selectedEntityType = SelectedEntityType::None;
    ResetMeshEditTracking();
}

void SceneDocument::DeleteSelectedObject() {
    if (selectedEntityType == SelectedEntityType::Mesh) {
        DeleteSelectedMesh();
    }
    else if (selectedEntityType == SelectedEntityType::Light) {
        light.enabled = false;
        ClearSelection();
    }
}

void SceneDocument::ExecuteCommand(std::unique_ptr<Command> cmd) {
    cmd->Execute();
    undoStack.push(std::move(cmd));
    while (!redoStack.empty()) {
        redoStack.pop();
    }
}

void SceneDocument::Undo() {
    if (undoStack.empty()) {
        return;
    }

    auto cmd = std::move(undoStack.top());
    undoStack.pop();
    cmd->Undo();
    redoStack.push(std::move(cmd));
}

void SceneDocument::Redo() {
    if (redoStack.empty()) {
        return;
    }

    auto cmd = std::move(redoStack.top());
    redoStack.pop();
    cmd->Execute();
    undoStack.push(std::move(cmd));
}

Mesh* SceneDocument::AddMesh(const std::shared_ptr<Mesh>& mesh) {
    Mesh* raw = mesh.get();
    meshes.push_back(mesh);
    return raw;
}

void SceneDocument::RemoveMesh(Mesh* mesh) {
    auto it = std::find_if(meshes.begin(), meshes.end(),
        [mesh](const auto& ptr) { return ptr.get() == mesh; });
    if (it == meshes.end()) {
        return;
    }

    meshes.erase(it);
    if (selectedMesh == mesh) {
        selectedMesh = nullptr;
        selectedEntityType = SelectedEntityType::None;
    }
    if (activeMeshEditTarget && activeMeshEditTarget.get() == mesh) {
        ResetMeshEditTracking();
    }
}

void SceneDocument::AddLightSource() {
    glm::vec3 lightPos = glm::vec3(3.0f, 4.0f, 3.0f);
    if (selectedMesh && selectedEntityType == SelectedEntityType::Mesh) {
        lightPos = selectedMesh->GetPosition() + glm::vec3(3.0f, 4.0f, 3.0f);
    }

    light.position = lightPos;
    light.enabled = true;
    ClearSelection();
}
