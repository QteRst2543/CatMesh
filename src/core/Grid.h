#pragma once

#include <glm/glm.hpp>

class Grid {
public:
    Grid();
    void Draw(const glm::mat4& view, const glm::mat4& projection);
    void DrawAxes(const glm::mat4& view, const glm::mat4& projection);
    void SetSize(int size) { gridSize = size; }

private:
    unsigned int VAO, VBO;
    unsigned int axesVAO, axesVBO;
    int vertexCount;
    int gridSize;
    void SetupGrid();
    void SetupAxes();
};