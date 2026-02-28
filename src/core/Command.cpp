#include "Command.h"
#include "app/Application.h"
#include "core/Mesh.h"

AddCubeCommand::AddCubeCommand(Application* app) : app(app), newMesh(nullptr) {}

void AddCubeCommand::Execute() {
    auto mesh = std::make_unique<Mesh>();
    mesh->LoadFromFile("");
    mesh->SetPosition(glm::vec3(0.0f));
    newMesh = app->AddMesh(std::move(mesh));
}

void AddCubeCommand::Undo() {
    if (newMesh) {
        app->RemoveMesh(newMesh);
        newMesh = nullptr;
    }
}

MoveCommand::MoveCommand(Mesh* mesh, const glm::vec3& newPos)
    : mesh(mesh), oldPos(mesh->GetPosition()), newPos(newPos) {
}

void MoveCommand::Execute() {
    if (mesh) mesh->SetPosition(newPos);
}

void MoveCommand::Undo() {
    if (mesh) mesh->SetPosition(oldPos);
}

ColorCommand::ColorCommand(Mesh* mesh, const glm::vec3& newColor)
    : mesh(mesh), oldColor(mesh->GetColor()), newColor(newColor) {
}

void ColorCommand::Execute() {
    if (mesh) mesh->SetColor(newColor);
}

void ColorCommand::Undo() {
    if (mesh) mesh->SetColor(oldColor);
}