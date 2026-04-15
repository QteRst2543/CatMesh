#include "Mesh.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <float.h>

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
position(0.0f), color(0.5f, 0.5f, 0.5f), editMode(EditMode::Object),
selectedFace(-1), selectedVertex(-1) {
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
    vertices = {
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

    indices = {
       0,1,2, 2,3,0,  // задняя
       4,5,6, 6,7,4,  // передняя
       8,9,10, 10,11,8,  // левая
       12,13,14, 14,15,12,  // правая
       16,17,18, 18,19,16,  // верхняя
       20,21,22, 22,23,20   // нижняя
    };

    // Создаем список граней
    faces.clear();
    faceCenters.clear();
    for (size_t i = 0; i < indices.size(); i += 3) {
        faces.push_back({ (int)indices[i], (int)indices[i + 1], (int)indices[i + 2] });

        // Вычисляем центр грани
        glm::vec3 center(0.0f);
        for (int j = 0; j < 3; j++) {
            center.x += vertices[indices[i + j] * 3];
            center.y += vertices[indices[i + j] * 3 + 1];
            center.z += vertices[indices[i + j] * 3 + 2];
        }
        center /= 3.0f;
        faceCenters.push_back(center);
    }

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

    glm::vec3 lightPos(3.0f, 3.0f, 3.0f);
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));

    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(color));

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// Реализация новых методов
void Mesh::SetEditMode(EditMode mode) {
    editMode = mode;
    selectedFace = -1;
    selectedVertex = -1;
    std::cout << "Edit mode changed to: " << (int)mode << std::endl;
}

glm::vec3 Mesh::GetFaceCenter(int faceIndex) const {
    if (faceIndex >= 0 && faceIndex < (int)faceCenters.size()) {
        return faceCenters[faceIndex];
    }
    return glm::vec3(0.0f);
}

glm::vec3 Mesh::GetVertexPosition(int vertexIndex) const {
    if (vertexIndex >= 0 && vertexIndex < (int)vertices.size() / 3) {
        return glm::vec3(vertices[vertexIndex * 3],
            vertices[vertexIndex * 3 + 1],
            vertices[vertexIndex * 3 + 2]);
    }
    return glm::vec3(0.0f);
}

void Mesh::UpdateSelectionFromMouse(const glm::vec3& rayOrigin, const glm::vec3& rayDir) {
    float closestDist = FLT_MAX;
    selectedFace = -1;
    selectedVertex = -1;

    if (editMode == EditMode::Face) {
        for (size_t i = 0; i < faces.size(); i++) {
            const auto& face = faces[i];
            glm::vec3 v0(vertices[face[0] * 3], vertices[face[0] * 3 + 1], vertices[face[0] * 3 + 2]);
            glm::vec3 v1(vertices[face[1] * 3], vertices[face[1] * 3 + 1], vertices[face[1] * 3 + 2]);
            glm::vec3 v2(vertices[face[2] * 3], vertices[face[2] * 3 + 1], vertices[face[2] * 3 + 2]);

            v0 += position;
            v1 += position;
            v2 += position;

            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 h = glm::cross(rayDir, edge2);
            float a = glm::dot(edge1, h);

            if (a > -0.0001f && a < 0.0001f) continue;

            float f = 1.0f / a;
            glm::vec3 s = rayOrigin - v0;
            float u = f * glm::dot(s, h);
            if (u < 0.0f || u > 1.0f) continue;

            glm::vec3 q = glm::cross(s, edge1);
            float v = f * glm::dot(rayDir, q);
            if (v < 0.0f || u + v > 1.0f) continue;

            float t = f * glm::dot(edge2, q);
            if (t > 0.0001f && t < closestDist) {
                closestDist = t;
                selectedFace = i;
            }
        }
        if (selectedFace >= 0) {
            std::cout << "Selected face: " << selectedFace << std::endl;
        }
    }
}

void Mesh::DragSelectedElement(const glm::vec3& delta) {
    if (editMode == EditMode::Face && selectedFace >= 0) {
        for (int idx : faces[selectedFace]) {
            vertices[idx * 3] += delta.x;
            vertices[idx * 3 + 1] += delta.y;
            vertices[idx * 3 + 2] += delta.z;
        }

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

        for (size_t i = 0; i < faces.size(); i++) {
            glm::vec3 center(0.0f);
            for (int idx : faces[i]) {
                center.x += vertices[idx * 3];
                center.y += vertices[idx * 3 + 1];
                center.z += vertices[idx * 3 + 2];
            }
            center /= 3.0f;
            faceCenters[i] = center;
        }
    }
}

bool Mesh::IntersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir) const {
    glm::vec3 min = position - glm::vec3(0.5f);
    glm::vec3 max = position + glm::vec3(0.5f);

    float t1 = (min.x - rayOrigin.x) / rayDir.x;
    float t2 = (max.x - rayOrigin.x) / rayDir.x;
    float t3 = (min.y - rayOrigin.y) / rayDir.y;
    float t4 = (max.y - rayOrigin.y) / rayDir.y;
    float t5 = (min.z - rayOrigin.z) / rayDir.z;
    float t6 = (max.z - rayOrigin.z) / rayDir.z;

    float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
    float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

    if (tmax < 0) return false;
    if (tmin > tmax) return false;

    return true;
}