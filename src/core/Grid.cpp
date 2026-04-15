#include "Grid.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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

void Grid::DrawAxes(const glm::mat4& view, const glm::mat4& projection) {
    static unsigned int shaderProgram = 0;
    if (!shaderProgram) {
        const char* vertexShader = "#version 460 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "layout (location = 1) in vec3 aColor;\n"
            "out vec3 Color;\n"
            "uniform mat4 view;\n"
            "uniform mat4 projection;\n"
            "void main() {\n"
            "    Color = aColor;\n"
            "    gl_Position = projection * view * vec4(aPos, 1.0);\n"
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
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(axesVAO);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);
}