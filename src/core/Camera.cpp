#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

Camera::Camera() : radius(3.0f), angleX(0.0f), angleY(glm::radians(30.0f)),
target(0.0f), up(0.0f, 1.0f, 0.0f) {
}

void Camera::Update() {
    position.x = target.x + radius * cos(angleY) * sin(angleX);
    position.y = target.y + radius * sin(angleY);
    position.z = target.z + radius * cos(angleY) * cos(angleX);
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(position, target, up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
}

void Camera::Rotate(float deltaX, float deltaY) {
    angleX += deltaX * 0.01f;
    angleY += deltaY * 0.01f;

    // Ограничиваем угол Y, чтобы не улететь под объект
    angleY = glm::clamp(angleY, -glm::half_pi<float>() + 0.1f, glm::half_pi<float>() - 0.1f);
    Update();
}

void Camera::Zoom(float delta) {
    radius -= delta * 0.1f;
    radius = glm::clamp(radius, 1.0f, 10.0f);
    Update();
}