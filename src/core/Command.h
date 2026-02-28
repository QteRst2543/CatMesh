#pragma once
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Platform/Window.h"
#include "ui/UI.h"
#include "core/Camera.h"
#include "core/Mesh.h"
#include <vector>
#include <stack>
#include "core/Command.h"

class Application;
class Mesh;

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
    Mesh* newMesh; // Сырой указатель, владение у Application
};

class MoveCommand : public Command {
public:
    MoveCommand(Mesh* mesh, const glm::vec3& newPos);
    void Execute() override;
    void Undo() override;
private:
    Mesh* mesh;
    glm::vec3 oldPos;
    glm::vec3 newPos;
};

class ColorCommand : public Command {
public:
    ColorCommand(Mesh* mesh, const glm::vec3& newColor);
    void Execute() override;
    void Undo() override;
private:
    Mesh* mesh;
    glm::vec3 oldColor;
    glm::vec3 newColor;
};