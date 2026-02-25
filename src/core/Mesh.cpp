#include "Mesh.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

//Временная затычка 
//
std::string Mesh::LoadShaderSource(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "Failed to open shader file: " << filepath << std::endl;
        return {};
    }

    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void Mesh::CreateShaderProgram()
{
    // Временные шейдеры прямо в коде
    const char* vertSource = R"(
        #version 460 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    const char* fragSource = R"(
        #version 460 core
        out vec4 FragColor;
        uniform vec3 color;
        void main() {
            FragColor = vec4(color, 1.0);
        }
    )";
}
//


Mesh::Mesh() : VAO(0), VBO(0), EBO(0), shaderProgram(0), vertexCount(0),
position(0.0f), color(1.0f, 0.5f, 0.8f) { // розовый 
    CreateShaderProgram();
}

Mesh::~Mesh() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}

bool Mesh::LoadFromFile(const std::string& filename) {
    // простой куб 
    std::vector<float> vertices = {
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f
    };

    std::vector<unsigned int> indices = {
        0,1,2, 2,3,0,  // задняя
        4,5,6, 6,7,4,  // передняя
        0,4,7, 7,3,0,  // левая
        1,5,6, 6,2,1,  // правая
        3,2,6, 6,7,3,  // верхняя
        0,1,5, 5,4,0   // нижняя
    };

    vertexCount = indices.size();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}
void Mesh::Draw(const glm::mat4& view, const glm::mat4& projection) {
    // Временно игнорируем шейдеры
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(projection));

    glMatrixMode(GL_MODELVIEW);
    glm::mat4 modelview = view * glm::translate(glm::mat4(1.0f), position);
    glLoadMatrixf(glm::value_ptr(modelview));

    glColor3f(1.0f, 0.0f, 0.0f);  // Ярко-красный

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);


}    //if (!shaderProgram) {
    //    std::cerr << "No shader program!" << std::endl;
    //    return;
    //}

    //glUseProgram(shaderProgram);
    //std::cout << "Drawing mesh with shader " << shaderProgram << std::endl;  // отладочный вывод

    //if (!VAO) {
    //    std::cerr << "No VAO!" << std::endl;
    //    return;
    //}
    //std::cout << "VAO: " << VAO << ", vertexCount: " << vertexCount << std::endl;


    //glm::mat4 model = glm::translate(glm::mat4(1.0f), position);

    //glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    //glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    //glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    //glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(color));

    //glBindVertexArray(VAO);
    //glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
    //glBindVertexArray(0);