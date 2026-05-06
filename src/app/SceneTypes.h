#pragma once

#include <glm/glm.hpp>

enum class SelectedEntityType {
    None,
    Mesh,
    Light
};

struct LightState {
    bool enabled = true;
    glm::vec3 position = glm::vec3(3.0f, 4.0f, 3.0f);
    glm::vec3 direction = glm::normalize(glm::vec3(-3.0f, -4.0f, -3.0f));
    glm::vec3 color = glm::vec3(1.0f, 0.96f, 0.9f);
    float intensity = 6.0f;
    float ambientStrength = 0.28f;
    float innerCutoffDegrees = 18.0f;
    float outerCutoffDegrees = 45.0f;
};
