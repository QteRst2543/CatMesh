#include "Mesh.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>

//Временная затычка 
//
std::string LoadFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::cout << "shaders ready\n";
    return buffer.str();
}

void Mesh::CreateShaderProgram()
{
    std::string vertexCode = LoadFile("shaders/basic.vert");
    std::string fragmentCode = LoadFile("shaders/basic.frag");

    const char* vertexShaderSource = vertexCode.c_str();
    const char* fragmentShaderSource = fragmentCode.c_str();
}
//


Mesh::Mesh() : VAO(0), VBO(0), EBO(0), shaderProgram(0), vertexCount(0),
position(0.0f), color(0.5f, 0.5f, 0.5f) { 
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
         //задняя
        -0.5f,-0.5f,-0.5f,0,0,-1,
        0.5f,-0.5f,-0.5f,0,0,-1,
        0.5f,0.5f,-0.5f,0,0,-1,
        -0.5f,0.5f,-0.5f,0,0,-1,
        //передняя
        -0.5f,-0.5f,0.5f,0,0,1,
        0.5f,-0.5f,0.5f,0,0,1,
        0.5f,0.5f,0.5f,0,0,1,
        -0.5f,0.5f,0.5f,0,0,1,
        //левая
        -0.5f,-0.5f,-0.5f,-1,0,0,
        -0.5f,0.5f,-0.5f,-1,0,0,
        -0.5f,0.5f,0.5f,-1,0,0,
        -0.5f,-0.5f,0.5f,-1,0,0,
        //правая
        0.5f,-0.5f,-0.5f,1,0,0,
        0.5f,0.5f,-0.5f,1,0,0,
        0.5f,0.5f,0.5f,1,0,0,
        0.5f,-0.5f,0.5f,1,0,0,
        //верхняя
        -0.5f,0.5f,-0.5f,0,1,0,
        0.5f,0.5f,-0.5f,0,1,0,
        0.5f,0.5f,0.5f,0,1,0,
        -0.5f,0.5f,0.5f,0,1,0,
        //нижняя
        -0.5f,-0.5f,-0.5f,0,-1,0,
        0.5f,-0.5f,-0.5f,0,-1,0,
        0.5f,-0.5f,0.5f,0,-1,0,
        -0.5f,-0.5f,0.5f,0,-1,0,
    };

     std::vector<unsigned int> indices = {
        0,1,2, 2,3,0,  // задняя
        4,5,6, 6,7,4,  // передняя
        8,9,10, 10,11,8,  // левая
        12,13,14, 14,15,12,  // правая
        16,17,18, 18,19,16,  // верхняя
        20,21,22, 22,23,20   // нижняя
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}
void Mesh::Draw(const glm::mat4& view, const glm::mat4& projection) {
    glUseProgram(shaderProgram);
    //свет
    glm::vec3 lightPos(3.0f, 3.0f, 3.0f);
    glUniform3fv(
        glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos)
    );

    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Используем цвет из переменной color
    glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(color));

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}