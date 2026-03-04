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

    const std::vector<float>& GetVertices() const { return vertices; }
    const std::vector<unsigned int>& GetIndices() const { return indices; }
        
    bool LoadFromFile(const std::string& filename);
    void Draw(const glm::mat4& view, const glm::mat4& projection);

    void SetPosition(const glm::vec3& pos) { position = pos; }
    void SetColor(const glm::vec3& col) { color = col; }

    // Добавляем геттеры
    glm::vec3 GetPosition() const { return position; }
    glm::vec3 GetColor() const { return color; }
   
private:
    unsigned int VAO, VBO, EBO;
    unsigned int shaderProgram;
    int vertexCount;

    std::vector<float>vertices;
    std::vector<unsigned int> indices;

    glm::vec3 position;
    glm::vec3 color;

    void CreateShaderProgram();
    std::string LoadShaderSource(const std::string& filepath);
};