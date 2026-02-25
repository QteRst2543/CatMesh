#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>  
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Mesh {
public:
    Mesh();
    ~Mesh();

    bool LoadFromFile(const std::string& filename);
    void Draw(const glm::mat4& view, const glm::mat4& projection);

    void SetPosition(const glm::vec3& pos) { position = pos; }
    void SetColor(const glm::vec3& col) { color = col; }

private:
    unsigned int VAO, VBO, EBO;
    unsigned int shaderProgram;
    int vertexCount;

    glm::vec3 position;
    glm::vec3 color;

    void CreateShaderProgram();
    std::string LoadShaderSource(const std::string& filepath);
};