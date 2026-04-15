#include "Command.h"
#include "app/Application.h"
#include "core/Mesh.h"

AddCubeCommand::AddCubeCommand(Application* app) : app(app) {}

void AddCubeCommand::Execute() {
    if (!newMesh) {
        newMesh = std::make_shared<Mesh>();
        newMesh->LoadFromFile("");
        newMesh->SetPosition(glm::vec3(0.0f));
        newMesh->SetColor(glm::vec3(0.5f, 0.5f, 0.5f));
        newMesh->SetEditMode(Mesh::EditMode::Object);
    }

    app->AddMesh(newMesh);
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
