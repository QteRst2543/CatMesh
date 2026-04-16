#include "Camera.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

Camera::Camera() : radius(5.0f), angleX(0.0f), angleY(glm::radians(30.0f)),
target(0.0f), up(0.0f, 1.0f, 0.0f) {
    Update();
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
    const float safeAspect = aspect > 0.0001f ? aspect : 1.0f;
    return glm::perspective(glm::radians(45.0f), safeAspect, 0.1f, 100.0f);
}

glm::vec3 Camera::GetForward() const {
    return glm::normalize(target - position);
}

glm::vec3 Camera::GetRight() const {
    return glm::normalize(glm::cross(GetForward(), up));
}

glm::vec3 Camera::GetUp() const {
    return glm::normalize(glm::cross(GetRight(), GetForward()));
}

void Camera::Rotate(float deltaX, float deltaY) {
    angleX += deltaX * 0.01f;
    angleY += deltaY * 0.01f;

    // Ограничение угла Y
    angleY = glm::clamp(angleY, -glm::half_pi<float>() + 0.1f, glm::half_pi<float>() - 0.1f);
    Update();
}

void Camera::Zoom(float delta) {
    radius -= delta * 0.1f;
    radius = glm::clamp(radius, 1.0f, 10.0f);
    Update();
}

void Camera::SetTarget(const glm::vec3& newTarget) {
    target = newTarget;
    Update();
}

void Camera::MoveTarget(const glm::vec3& delta) {
    target += delta;
    Update();
}

void Camera::MoveTargetLocal(float forwardAmount, float rightAmount) {
    glm::vec3 forward = GetForward();
    glm::vec3 right = GetRight();

    forward.y = 0.0f;
    right.y = 0.0f;

    if (glm::length(forward) > 0.0001f) {
        forward = glm::normalize(forward);
    }
    if (glm::length(right) > 0.0001f) {
        right = glm::normalize(right);
    }

    target += forward * forwardAmount + right * rightAmount;
    Update();
}
