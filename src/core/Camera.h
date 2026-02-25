#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera {
public:
    Camera();

    void Update();
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspect) const;

    void Rotate(float deltaX, float deltaY);
    void Zoom(float delta);

private:
    float radius;
    float angleX;
    float angleY;
    glm::vec3 target;

    glm::vec3 position;
    glm::vec3 up;
};