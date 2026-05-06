#include "Command.h"
#include "app/Application.h"
#include "core/Mesh.h"

namespace {

glm::vec3 ComputeNewCubePosition(const Application& app) {
    if (const Mesh* selectedMesh = app.GetSelectedMesh()) {
        return selectedMesh->GetPosition() + glm::vec3(1.5f, 0.0f, 0.0f);
    }

    const auto& meshes = app.GetMeshes();
    if (!meshes.empty() && meshes.back()) {
        return meshes.back()->GetPosition() + glm::vec3(1.5f, 0.0f, 0.0f);
    }

    return glm::vec3(0.0f);
}

} // namespace

AddCubeCommand::AddCubeCommand(Application* app) : app(app) {}

void AddCubeCommand::Execute() {
    if (!newMesh) {
        newMesh = std::make_shared<Mesh>();
        newMesh->LoadFromFile("");
        newMesh->SetPosition(ComputeNewCubePosition(*app));
        newMesh->SetColor(glm::vec3(0.5f, 0.5f, 0.5f));
        newMesh->SetEditMode(Mesh::EditMode::Object);
    }

    app->AddMesh(newMesh);
    app->SelectMesh(newMesh.get());
}

void AddCubeCommand::Undo() {
    if (newMesh) {
        app->RemoveMesh(newMesh.get());
    }
}

MoveCommand::MoveCommand(const std::shared_ptr<Mesh>& mesh, const glm::vec3& newPos)
    : mesh(mesh), oldPos(mesh ? mesh->GetPosition() : glm::vec3(0.0f)), newPos(newPos) {
}

void MoveCommand::Execute() {
    if (auto lockedMesh = mesh.lock()) {
        lockedMesh->SetPosition(newPos);
    }
}

void MoveCommand::Undo() {
    if (auto lockedMesh = mesh.lock()) {
        lockedMesh->SetPosition(oldPos);
    }
}

ColorCommand::ColorCommand(const std::shared_ptr<Mesh>& mesh, const glm::vec3& newColor)
    : mesh(mesh), oldColor(mesh ? mesh->GetColor() : glm::vec3(0.0f)), newColor(newColor) {
}

void ColorCommand::Execute() {
    if (auto lockedMesh = mesh.lock()) {
        lockedMesh->SetColor(newColor);
    }
}

void ColorCommand::Undo() {
    if (auto lockedMesh = mesh.lock()) {
        lockedMesh->SetColor(oldColor);
    }
}

MeshStateCommand::MeshStateCommand(const std::shared_ptr<Mesh>& mesh, Mesh::State beforeState, Mesh::State afterState)
    : mesh(mesh), beforeState(std::move(beforeState)), afterState(std::move(afterState)) {
}

void MeshStateCommand::Execute() {
    if (auto lockedMesh = mesh.lock()) {
        lockedMesh->RestoreState(afterState);
    }
}

void MeshStateCommand::Undo() {
    if (auto lockedMesh = mesh.lock()) {
        lockedMesh->RestoreState(beforeState);
    }
}
