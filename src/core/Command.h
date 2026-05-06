#pragma once
#include <memory>
#include <glm/glm.hpp>
#include "core/Mesh.h"

class Application;

class Command {
public:
    virtual ~Command() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
};

class AddCubeCommand : public Command {
public:
    AddCubeCommand(Application* app);
    void Execute() override;
    void Undo() override;
private:
    Application* app;
    std::shared_ptr<Mesh> newMesh;
};

class MoveCommand : public Command {
public:
    MoveCommand(const std::shared_ptr<Mesh>& mesh, const glm::vec3& newPos);
    void Execute() override;
    void Undo() override;
private:
    std::weak_ptr<Mesh> mesh;
    glm::vec3 oldPos;
    glm::vec3 newPos;
};

class ColorCommand : public Command {
public:
    ColorCommand(const std::shared_ptr<Mesh>& mesh, const glm::vec3& newColor);
    void Execute() override;
    void Undo() override;
private:
    std::weak_ptr<Mesh> mesh;
    glm::vec3 oldColor;
    glm::vec3 newColor;
};

class MeshStateCommand : public Command {
public:
    MeshStateCommand(const std::shared_ptr<Mesh>& mesh, Mesh::State beforeState, Mesh::State afterState);
    void Execute() override;
    void Undo() override;
private:
    std::weak_ptr<Mesh> mesh;
    Mesh::State beforeState;
    Mesh::State afterState;
};
