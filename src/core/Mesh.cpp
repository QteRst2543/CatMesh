#include "Mesh.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>

std::string LoadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

unsigned int CompileShader(unsigned int type, const std::string& source) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void Mesh::CreateShaderProgram() {
    std::string vertexCode = LoadFile("shaders/basic.vert");
    std::string fragmentCode = LoadFile("shaders/basic.frag");

    if (vertexCode.empty() || fragmentCode.empty()) {
        std::cerr << "Failed to load shader files!" << std::endl;
        return;
    }

    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexCode);
    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentCode);

    if (!vertexShader || !fragmentShader) {
        std::cerr << "Shader compilation failed!" << std::endl;
        return;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader linking failed: " << infoLog << std::endl;
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::cout << "Shader program created successfully: " << shaderProgram << std::endl;
}

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
       -0.5f,-0.5f,-0.5f, 0,0,-1,
       0.5f,-0.5f,-0.5f, 0,0,-1,
       0.5f,0.5f,-0.5f, 0,0,-1,
       -0.5f,0.5f,-0.5f, 0,0,-1,
       //передняя
       -0.5f,-0.5f,0.5f, 0,0,1,
       0.5f,-0.5f,0.5f, 0,0,1,
       0.5f,0.5f,0.5f, 0,0,1,
       -0.5f,0.5f,0.5f, 0,0,1,
       //левая
       -0.5f,-0.5f,-0.5f, -1,0,0,
       -0.5f,0.5f,-0.5f, -1,0,0,
       -0.5f,0.5f,0.5f, -1,0,0,
       -0.5f,-0.5f,0.5f, -1,0,0,
       //правая
       0.5f,-0.5f,-0.5f, 1,0,0,
       0.5f,0.5f,-0.5f, 1,0,0,
       0.5f,0.5f,0.5f, 1,0,0,
       0.5f,-0.5f,0.5f, 1,0,0,
       //верхняя
       -0.5f,0.5f,-0.5f, 0,1,0,
       0.5f,0.5f,-0.5f, 0,1,0,
       0.5f,0.5f,0.5f, 0,1,0,
       -0.5f,0.5f,0.5f, 0,1,0,
       //нижняя
       -0.5f,-0.5f,-0.5f, 0,-1,0,
       0.5f,-0.5f,-0.5f, 0,-1,0,
       0.5f,-0.5f,0.5f, 0,-1,0,
       -0.5f,-0.5f,0.5f, 0,-1,0,
    };

    // Сохраняем вершины для экспорта STL
    this->vertices = vertices;

    std::vector<unsigned int> indices = {
       0,1,2, 2,3,0,  // задняя
       4,5,6, 6,7,4,  // передняя
       8,9,10, 10,11,8,  // левая
       12,13,14, 14,15,12,  // правая
       16,17,18, 18,19,16,  // верхняя
       20,21,22, 22,23,20   // нижняя
    };

    // Сохраняем индексы для экспорта STL
    this->indices = indices;

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

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::cout << "Cube loaded successfully. Vertex count: " << vertexCount << std::endl;
    return true;
}

void Mesh::Draw(const glm::mat4& view, const glm::mat4& projection) {
    if (!shaderProgram) {
        std::cerr << "No shader program!" << std::endl;
        return;
    }

    glUseProgram(shaderProgram);

    //свет
    glm::vec3 lightPos(3.0f, 3.0f, 3.0f);
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));

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