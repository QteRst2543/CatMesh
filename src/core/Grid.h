#pragma once

#include <glm/glm.hpp>

class Grid {
public:
    Grid();
    void Draw(const glm::mat4& view, const glm::mat4& projection);
    void DrawAxes(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& origin, float scale = 1.0f);
    void DrawLightCone(
        const glm::mat4& view, 
        const glm::mat4& projection,
        const glm::vec3& lightPos,
        const glm::vec3& lightDir,
        float innerCutoffDegrees,
        float outerCutoffDegrees,
        float coneLength = 10.0f);
    void DrawRotationGizmo(
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& center,
        int axisMode);
    void SetSize(int size) { gridSize = size; }

private:
    unsigned int VAO, VBO;
    unsigned int axesVAO, axesVBO;
    int vertexCount;
    int gridSize;
    void SetupGrid();
    void SetupAxes();
};
