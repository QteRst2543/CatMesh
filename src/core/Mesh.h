#pragma once
#include <vector>
#include <string>
#include <optional>
#include <glm/glm.hpp>  
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Mesh {
public:
    enum class EditMode { Object, Vertex, Edge, Face };

    Mesh();
    ~Mesh();

    const std::vector<float>& GetVertices() const { return vertices; }
    const std::vector<unsigned int>& GetIndices() const { return indices; }
    const std::vector<glm::vec3>& GetEditableVertices() const { return editableVertices; }
    const std::vector<std::vector<int>>& GetFaces() const { return faces; }

    bool LoadFromFile(const std::string& filename);
    bool SetGeometry(const std::vector<glm::vec3>& newVertices, const std::vector<std::vector<int>>& newFaces);
    void Draw(
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& cameraPosition,
        const glm::vec3& lightPosition,
        const glm::vec3& lightDirection,
        const glm::vec3& lightColor,
        float lightIntensity,
        float ambientStrength,
        float innerCutoffDegrees,
        float outerCutoffDegrees
    );

    void SetPosition(const glm::vec3& pos) { position = pos; }
    void SetColor(const glm::vec3& col) { color = col; }

    glm::vec3 GetPosition() const { return position; }
    glm::vec3 GetColor() const { return color; }

    // Новые методы для редактирования - ТОЛЬКО ОБЪЯВЛЕНИЯ, без реализации
    void SetEditMode(EditMode mode);
    EditMode GetEditMode() const { return editMode; }
    void SetSelectedFace(int faceIndex) { selectedFace = faceIndex; }
    int GetSelectedFace() const { return selectedFace; }
    void SetSelectedVertex(int vertexIndex) { selectedVertex = vertexIndex; }
    int GetSelectedVertex() const { return selectedVertex; }

    void UpdateSelectionFromMouse(const glm::vec3& rayOrigin, const glm::vec3& rayDir);
    void DragSelectedElement(const glm::vec3& delta);
    glm::vec3 GetFaceCenter(int faceIndex) const;
    glm::vec3 GetVertexPosition(int vertexIndex) const;
    bool IntersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float* hitDistance = nullptr) const;

private:
    unsigned int VAO, VBO, EBO;
    unsigned int shaderProgram;
    int vertexCount;

    std::vector<glm::vec3> editableVertices;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::vector<int>> faces;
    std::vector<glm::vec3> faceCenters;

    glm::vec3 position;
    glm::vec3 color;

    EditMode editMode = EditMode::Object;
    int selectedFace = -1;
    int selectedVertex = -1;

    void CreateShaderProgram();
    void RebuildFaceCenters();
    void RebuildRenderData();
    void UploadBuffers();
};
