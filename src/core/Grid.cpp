#include "Grid.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <vector>
#include <iostream>

Grid::Grid() : VAO(0), VBO(0), axesVAO(0), axesVBO(0), vertexCount(0), gridSize(20) {
    SetupGrid();
    SetupAxes();
}

void Grid::SetupGrid() {
    std::vector<float> vertices;

    // Горизонтальные линии
    for (int i = -gridSize; i <= gridSize; i++) {
        vertices.push_back(-gridSize);
        vertices.push_back(0.0f);
        vertices.push_back(i);

        vertices.push_back(gridSize);
        vertices.push_back(0.0f);
        vertices.push_back(i);
    }

    // Вертикальные линии
    for (int i = -gridSize; i <= gridSize; i++) {
        vertices.push_back(i);
        vertices.push_back(0.0f);
        vertices.push_back(-gridSize);

        vertices.push_back(i);
        vertices.push_back(0.0f);
        vertices.push_back(gridSize);
    }

    vertexCount = vertices.size() / 3;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Grid::SetupAxes() {
    // Оси X (красная), Y (зеленая), Z (синяя)
    std::vector<float> vertices = {
        // X axis (red)
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        // Y axis (green)
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 2.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        // Z axis (blue)
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 1.0f
    };

    glGenVertexArrays(1, &axesVAO);
    glGenBuffers(1, &axesVBO);

    glBindVertexArray(axesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Grid::Draw(const glm::mat4& view, const glm::mat4& projection) {
    // Создаем простой шейдер для сетки
    static unsigned int shaderProgram = 0;
    if (!shaderProgram) {
        const char* vertexShader = "#version 460 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "uniform mat4 view;\n"
            "uniform mat4 projection;\n"
            "void main() {\n"
            "    gl_Position = projection * view * vec4(aPos, 1.0);\n"
            "}\n";

        const char* fragmentShader = "#version 460 core\n"
            "out vec4 FragColor;\n"
            "void main() {\n"
            "    FragColor = vec4(0.3, 0.3, 0.3, 1.0);\n"
            "}\n";

        unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertexShader, nullptr);
        glCompileShader(vs);

        int success;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(vs, 512, nullptr, infoLog);
            std::cerr << "Grid vertex shader compilation failed: " << infoLog << std::endl;
        }

        unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragmentShader, nullptr);
        glCompileShader(fs);

        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(fs, 512, nullptr, infoLog);
            std::cerr << "Grid fragment shader compilation failed: " << infoLog << std::endl;
        }

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vs);
        glAttachShader(shaderProgram, fs);
        glLinkProgram(shaderProgram);

        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
            std::cerr << "Grid shader linking failed: " << infoLog << std::endl;
        }

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, vertexCount);
    glBindVertexArray(0);
}

void Grid::DrawAxes(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& origin, float scale) {
    static unsigned int shaderProgram = 0;
    if (!shaderProgram) {
        const char* vertexShader = "#version 460 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "layout (location = 1) in vec3 aColor;\n"
            "out vec3 Color;\n"
            "uniform mat4 model;\n"
            "uniform mat4 view;\n"
            "uniform mat4 projection;\n"
            "void main() {\n"
            "    Color = aColor;\n"
            "    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
            "}\n";

        const char* fragmentShader = "#version 460 core\n"
            "in vec3 Color;\n"
            "out vec4 FragColor;\n"
            "void main() {\n"
            "    FragColor = vec4(Color, 1.0);\n"
            "}\n";

        unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertexShader, nullptr);
        glCompileShader(vs);

        unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragmentShader, nullptr);
        glCompileShader(fs);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vs);
        glAttachShader(shaderProgram, fs);
        glLinkProgram(shaderProgram);

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    glUseProgram(shaderProgram);
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), origin) *
        glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(axesVAO);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);
}

void Grid::DrawLightCone(
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& lightPos,
    const glm::vec3& lightDir,
    float innerCutoffDegrees,
    float outerCutoffDegrees,
    float coneLength) {
    static unsigned int shaderProgram = 0;
    if (!shaderProgram) {
        const char* vertexShader = "#version 460 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "uniform mat4 view;\n"
            "uniform mat4 projection;\n"
            "void main() {\n"
            "    gl_Position = projection * view * vec4(aPos, 1.0);\n"
            "}\n";

        const char* fragmentShader = "#version 460 core\n"
            "out vec4 FragColor;\n"
            "void main() {\n"
            "    FragColor = vec4(1.0, 1.0, 0.2, 1.0);\n"
            "}\n";

        unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertexShader, nullptr);
        glCompileShader(vs);

        unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragmentShader, nullptr);
        glCompileShader(fs);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vs);
        glAttachShader(shaderProgram, fs);
        glLinkProgram(shaderProgram);

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    glm::vec3 right = glm::normalize(glm::cross(lightDir, glm::vec3(0.0f, 1.0f, 0.0f)));
    if (glm::length(right) < 0.0001f) {
        right = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    glm::vec3 up = glm::normalize(glm::cross(right, lightDir));

    float outerRadiusAngle = glm::radians(outerCutoffDegrees);
    float outerRadius = coneLength * glm::tan(outerRadiusAngle);

    std::vector<float> vertices;
    std::vector<float> circleVertices;

    // Center ray
    vertices.push_back(lightPos.x);
    vertices.push_back(lightPos.y);
    vertices.push_back(lightPos.z);
    glm::vec3 centerEnd = lightPos + lightDir * coneLength;
    vertices.push_back(centerEnd.x);
    vertices.push_back(centerEnd.y);
    vertices.push_back(centerEnd.z);

    // Four parallel side rays around the center
    const float offsetDistance = outerRadius * 0.35f;
    std::array<glm::vec3, 4> sideOffsets = {
        right * offsetDistance,
        -right * offsetDistance,
        up * offsetDistance,
        -up * offsetDistance
    };

    for (const auto& offset : sideOffsets) {
        glm::vec3 start = lightPos + offset;
        glm::vec3 end = start + lightDir * coneLength;
        vertices.push_back(start.x);
        vertices.push_back(start.y);
        vertices.push_back(start.z);
        vertices.push_back(end.x);
        vertices.push_back(end.y);
        vertices.push_back(end.z);
    }

    // Outer cone edge rays
    for (int i = 0; i < 2; i++) {
        float angle = glm::pi<float>() * i;
        glm::vec3 edgeDir = glm::cos(angle) * right + glm::sin(angle) * up;
        glm::vec3 endPoint = lightPos + lightDir * coneLength + edgeDir * outerRadius;
        vertices.push_back(lightPos.x);
        vertices.push_back(lightPos.y);
        vertices.push_back(lightPos.z);
        vertices.push_back(endPoint.x);
        vertices.push_back(endPoint.y);
        vertices.push_back(endPoint.z);
    }

    // Circle around light position
    const int circleSegments = 24;
    const float circleRadius = glm::max(0.2f, outerRadius * 0.15f);
    for (int i = 0; i < circleSegments; i++) {
        float theta = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(circleSegments);
        glm::vec3 point = lightPos + right * glm::cos(theta) * circleRadius + up * glm::sin(theta) * circleRadius;
        circleVertices.push_back(point.x);
        circleVertices.push_back(point.y);
        circleVertices.push_back(point.z);
    }

    const int lineVertexCount = static_cast<int>(vertices.size() / 3);
    vertices.insert(vertices.end(), circleVertices.begin(), circleVertices.end());
    const int circleVertexCount = static_cast<int>(circleVertices.size() / 3);

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glDrawArrays(GL_LINES, 0, lineVertexCount);
    if (circleVertexCount > 0) {
        glDrawArrays(GL_LINE_LOOP, lineVertexCount, circleVertexCount);
    }

    glPointSize(10.0f);
    glDrawArrays(GL_POINTS, 0, 1);

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

void Grid::DrawRotationGizmo(
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& center,
    int axisMode) {
    static unsigned int shaderProgram = 0;
    if (!shaderProgram) {
        const char* vertexShader = "#version 460 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "uniform mat4 view;\n"
            "uniform mat4 projection;\n"
            "void main() {\n"
            "    gl_Position = projection * view * vec4(aPos, 1.0);\n"
            "}\n";

        const char* fragmentShader = "#version 460 core\n"
            "out vec4 FragColor;\n"
            "void main() {\n"
            "    FragColor = vec4(1.0, 0.5, 0.0, 1.0);\n"  // Orange color
            "}\n";

        unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertexShader, nullptr);
        glCompileShader(vs);

        unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragmentShader, nullptr);
        glCompileShader(fs);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vs);
        glAttachShader(shaderProgram, fs);
        glLinkProgram(shaderProgram);

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    glm::vec3 right, up;
    if (axisMode == 0) {  // X-axis: circle in YZ plane
        right = glm::vec3(0, 1, 0);
        up = glm::vec3(0, 0, 1);
    } else if (axisMode == 1) {  // Y-axis: circle in XZ plane
        right = glm::vec3(1, 0, 0);
        up = glm::vec3(0, 0, 1);
    } else {  // Z-axis: circle in XY plane
        right = glm::vec3(1, 0, 0);
        up = glm::vec3(0, 1, 0);
    }

    const int segments = 32;
    const float radius = 0.9f;
    std::vector<float> vertices;

    for (int i = 0; i < segments; i++) {
        float theta = 2.0f * glm::pi<float>() * i / segments;
        glm::vec3 point = center + right * glm::cos(theta) * radius + up * glm::sin(theta) * radius;
        vertices.push_back(point.x);
        vertices.push_back(point.y);
        vertices.push_back(point.z);
    }

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glDrawArrays(GL_LINE_LOOP, 0, segments);

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}
