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
    glm::vec3 GetPosition() const { return position; }

    void Rotate(float deltaX, float deltaY);
    void Zoom(float delta);

    // Добавьте этот метод
    void SetMousePosition(float x, float y, int screenWidth, int screenHeight) {
        // Пока не используется, но нужен для компиляции
        lastMouseX = x;
        lastMouseY = y;
    }

private:
    float radius;
    float angleX;
    float angleY;
    glm::vec3 target;
    glm::vec3 position;
    glm::vec3 up;

    float lastMouseX, lastMouseY; // Добавьте эти поля
};