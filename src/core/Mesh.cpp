#include "Mesh.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cfloat>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int kRenderStride = 6;

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

    int success = 0;
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

glm::vec3 SafeNormalize(const glm::vec3& value) {
    const float len = glm::length(value);
    if (len <= 0.00001f) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }
    return value / len;
}

std::vector<glm::vec3> CreateDefaultCubeVertices() {
    return {
        {-0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f},
        {-0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f},
        { 0.5f,  0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f},
    };
}

std::vector<std::vector<int>> CreateDefaultCubeFaces() {
    return {
        {0, 1, 2, 3},
        {4, 5, 6, 7},
        {0, 4, 7, 3},
        {1, 5, 6, 2},
        {3, 2, 6, 7},
        {0, 1, 5, 4},
    };
}

std::optional<int> ParseObjVertexIndex(const std::string& token, std::size_t vertexCount) {
    if (token.empty()) {
        return std::nullopt;
    }

    const std::size_t slashPos = token.find('/');
    const std::string indexToken = token.substr(0, slashPos);
    if (indexToken.empty()) {
        return std::nullopt;
    }

    int rawIndex = 0;
    try {
        rawIndex = std::stoi(indexToken);
    }
    catch (...) {
        return std::nullopt;
    }

    if (rawIndex > 0) {
        rawIndex -= 1;
    }
    else if (rawIndex < 0) {
        rawIndex = static_cast<int>(vertexCount) + rawIndex;
    }

    if (rawIndex < 0 || rawIndex >= static_cast<int>(vertexCount)) {
        return std::nullopt;
    }

    return rawIndex;
}

bool LoadObjGeometry(
    const std::string& path,
    std::vector<glm::vec3>& outVertices,
    std::vector<std::vector<int>>& outFaces
) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << path << std::endl;
        return false;
    }

    std::vector<glm::vec3> parsedVertices;
    std::vector<std::vector<int>> parsedFaces;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream lineStream(line);
        std::string prefix;
        lineStream >> prefix;

        if (prefix == "v") {
            glm::vec3 vertex(0.0f);
            if (!(lineStream >> vertex.x >> vertex.y >> vertex.z)) {
                std::cerr << "Malformed OBJ vertex line: " << line << std::endl;
                return false;
            }
            parsedVertices.push_back(vertex);
        }
        else if (prefix == "f") {
            std::vector<int> face;
            std::string token;
            while (lineStream >> token) {
                const auto parsedIndex = ParseObjVertexIndex(token, parsedVertices.size());
                if (!parsedIndex.has_value()) {
                    std::cerr << "Malformed OBJ face token: " << token << std::endl;
                    return false;
                }
                face.push_back(*parsedIndex);
            }

            if (face.size() < 3) {
                std::cerr << "OBJ face must have at least 3 vertices." << std::endl;
                return false;
            }

            parsedFaces.push_back(std::move(face));
        }
    }

    if (parsedVertices.empty() || parsedFaces.empty()) {
        std::cerr << "OBJ file does not contain geometry: " << path << std::endl;
        return false;
    }

    outVertices = std::move(parsedVertices);
    outFaces = std::move(parsedFaces);
    return true;
}

bool IntersectTriangle(
    const glm::vec3& rayOrigin,
    const glm::vec3& rayDir,
    const glm::vec3& v0,
    const glm::vec3& v1,
    const glm::vec3& v2,
    float& hitDistance
) {
    const glm::vec3 edge1 = v1 - v0;
    const glm::vec3 edge2 = v2 - v0;
    const glm::vec3 h = glm::cross(rayDir, edge2);
    const float a = glm::dot(edge1, h);

    if (std::fabs(a) < 0.0001f) {
        return false;
    }

    const float f = 1.0f / a;
    const glm::vec3 s = rayOrigin - v0;
    const float u = f * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    const glm::vec3 q = glm::cross(s, edge1);
    const float v = f * glm::dot(rayDir, q);
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    const float t = f * glm::dot(edge2, q);
    if (t <= 0.0001f) {
        return false;
    }

    hitDistance = t;
    return true;
}

} // namespace

void Mesh::CreateShaderProgram() {
    std::string vertexCode = LoadFile("shaders/basic.vert");
    std::string fragmentCode = LoadFile("shaders/basic.frag");

    if (vertexCode.empty() || fragmentCode.empty()) {
        std::cerr << "Failed to load shader files!" << std::endl;
        return;
    }

    const unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexCode);
    const unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentCode);

    if (!vertexShader || !fragmentShader) {
        std::cerr << "Shader compilation failed!" << std::endl;
        return;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success = 0;
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
}

Mesh::Mesh()
    : VAO(0),
      VBO(0),
      EBO(0),
      shaderProgram(0),
      vertexCount(0),
      position(0.0f),
      color(0.5f, 0.5f, 0.5f),
      editMode(EditMode::Object),
      selectedFace(-1),
      selectedVertex(-1) {
    CreateShaderProgram();
}

Mesh::~Mesh() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}

bool Mesh::LoadFromFile(const std::string& filename) {
    if (filename.empty()) {
        return SetGeometry(CreateDefaultCubeVertices(), CreateDefaultCubeFaces());
    }

    std::filesystem::path filePath(filename);
    std::string extension = filePath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (extension == ".obj") {
        std::vector<glm::vec3> objVertices;
        std::vector<std::vector<int>> objFaces;
        if (!LoadObjGeometry(filename, objVertices, objFaces)) {
            return false;
        }
        return SetGeometry(objVertices, objFaces);
    }

    std::cerr << "Unsupported mesh format: " << filename << std::endl;
    return false;
}

bool Mesh::SetGeometry(const std::vector<glm::vec3>& newVertices, const std::vector<std::vector<int>>& newFaces) {
    if (newVertices.empty() || newFaces.empty()) {
        std::cerr << "Mesh geometry cannot be empty." << std::endl;
        return false;
    }

    for (const auto& face : newFaces) {
        if (face.size() < 3) {
            std::cerr << "Each face must contain at least 3 vertices." << std::endl;
            return false;
        }

        for (int index : face) {
            if (index < 0 || index >= static_cast<int>(newVertices.size())) {
                std::cerr << "Face index is out of range." << std::endl;
                return false;
            }
        }
    }

    editableVertices = newVertices;
    faces = newFaces;
    selectedFace = -1;
    selectedVertex = -1;

    RebuildFaceCenters();
    RebuildRenderData();
    UploadBuffers();
    return true;
}

void Mesh::RebuildFaceCenters() {
    faceCenters.clear();
    faceCenters.reserve(faces.size());

    for (const auto& face : faces) {
        glm::vec3 center(0.0f);
        for (int vertexIndex : face) {
            center += editableVertices[vertexIndex];
        }
        center /= static_cast<float>(face.size());
        faceCenters.push_back(center);
    }
}

void Mesh::RebuildRenderData() {
    vertices.clear();
    indices.clear();

    unsigned int nextIndex = 0;
    for (const auto& face : faces) {
        for (std::size_t i = 1; i + 1 < face.size(); ++i) {
            const glm::vec3& p0 = editableVertices[face[0]];
            const glm::vec3& p1 = editableVertices[face[i]];
            const glm::vec3& p2 = editableVertices[face[i + 1]];
            const glm::vec3 normal = SafeNormalize(glm::cross(p1 - p0, p2 - p0));

            const glm::vec3 triangleVertices[3] = { p0, p1, p2 };
            for (const glm::vec3& vertex : triangleVertices) {
                vertices.push_back(vertex.x);
                vertices.push_back(vertex.y);
                vertices.push_back(vertex.z);
                vertices.push_back(normal.x);
                vertices.push_back(normal.y);
                vertices.push_back(normal.z);
                indices.push_back(nextIndex++);
            }
        }
    }

    vertexCount = static_cast<int>(indices.size());
}

void Mesh::UploadBuffers() {
    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    }

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, kRenderStride * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, kRenderStride * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Mesh::Draw(
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
) {
    if (!shaderProgram || vertexCount == 0) {
        return;
    }

    glUseProgram(shaderProgram);

    const glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(color));
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPosition));
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPosition));
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightDir"), 1, glm::value_ptr(lightDirection));
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform1f(glGetUniformLocation(shaderProgram, "lightIntensity"), lightIntensity);
    glUniform1f(glGetUniformLocation(shaderProgram, "ambientStrength"), ambientStrength);
    glUniform1f(glGetUniformLocation(shaderProgram, "innerCutoff"), glm::cos(glm::radians(innerCutoffDegrees)));
    glUniform1f(glGetUniformLocation(shaderProgram, "outerCutoff"), glm::cos(glm::radians(outerCutoffDegrees)));

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::SetEditMode(EditMode mode) {
    editMode = mode;
    selectedFace = -1;
    selectedVertex = -1;
}

glm::vec3 Mesh::GetFaceCenter(int faceIndex) const {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(faceCenters.size())) {
        return faceCenters[faceIndex];
    }
    return glm::vec3(0.0f);
}

glm::vec3 Mesh::GetVertexPosition(int vertexIndex) const {
    if (vertexIndex >= 0 && vertexIndex < static_cast<int>(editableVertices.size())) {
        return editableVertices[vertexIndex];
    }
    return glm::vec3(0.0f);
}

void Mesh::UpdateSelectionFromMouse(const glm::vec3& rayOrigin, const glm::vec3& rayDir) {
    selectedFace = -1;
    selectedVertex = -1;

    if (editMode == EditMode::Face) {
        float closestDistance = std::numeric_limits<float>::max();

        for (std::size_t faceIndex = 0; faceIndex < faces.size(); ++faceIndex) {
            const auto& face = faces[faceIndex];
            for (std::size_t i = 1; i + 1 < face.size(); ++i) {
                const glm::vec3 v0 = editableVertices[face[0]] + position;
                const glm::vec3 v1 = editableVertices[face[i]] + position;
                const glm::vec3 v2 = editableVertices[face[i + 1]] + position;

                float hitDistance = 0.0f;
                if (IntersectTriangle(rayOrigin, rayDir, v0, v1, v2, hitDistance) &&
                    hitDistance < closestDistance) {
                    closestDistance = hitDistance;
                    selectedFace = static_cast<int>(faceIndex);
                }
            }
        }
    }
    else if (editMode == EditMode::Vertex) {
        float closestDistance = std::numeric_limits<float>::max();

        for (std::size_t vertexIndex = 0; vertexIndex < editableVertices.size(); ++vertexIndex) {
            const glm::vec3 worldVertex = editableVertices[vertexIndex] + position;
            const float projection = glm::dot(worldVertex - rayOrigin, rayDir);
            if (projection <= 0.0f) {
                continue;
            }

            const glm::vec3 nearestPoint = rayOrigin + rayDir * projection;
            const float rayDistance = glm::length(worldVertex - nearestPoint);
            const float threshold = 0.08f + projection * 0.02f;
            if (rayDistance < threshold && rayDistance < closestDistance) {
                closestDistance = rayDistance;
                selectedVertex = static_cast<int>(vertexIndex);
            }
        }
    }
}

void Mesh::DragSelectedElement(const glm::vec3& delta) {
    if (editMode == EditMode::Face && selectedFace >= 0 && selectedFace < static_cast<int>(faces.size())) {
        std::set<int> uniqueVertices(faces[selectedFace].begin(), faces[selectedFace].end());
        for (int vertexIndex : uniqueVertices) {
            editableVertices[vertexIndex] += delta;
        }
    }
    else if (editMode == EditMode::Vertex && selectedVertex >= 0 &&
             selectedVertex < static_cast<int>(editableVertices.size())) {
        editableVertices[selectedVertex] += delta;
    }
    else {
        return;
    }

    RebuildFaceCenters();
    RebuildRenderData();
    UploadBuffers();
}

bool Mesh::IntersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float* hitDistance) const {
    if (editableVertices.empty()) {
        return false;
    }

    glm::vec3 minBounds = editableVertices.front() + position;
    glm::vec3 maxBounds = editableVertices.front() + position;
    for (const glm::vec3& vertex : editableVertices) {
        const glm::vec3 worldVertex = vertex + position;
        minBounds = glm::min(minBounds, worldVertex);
        maxBounds = glm::max(maxBounds, worldVertex);
    }

    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::max();

    for (int axis = 0; axis < 3; ++axis) {
        const float origin = rayOrigin[axis];
        const float direction = rayDir[axis];
        const float minValue = minBounds[axis];
        const float maxValue = maxBounds[axis];

        if (std::fabs(direction) < 0.00001f) {
            if (origin < minValue || origin > maxValue) {
                return false;
            }
            continue;
        }

        float axisMin = (minValue - origin) / direction;
        float axisMax = (maxValue - origin) / direction;
        if (axisMin > axisMax) {
            std::swap(axisMin, axisMax);
        }

        tMin = std::max(tMin, axisMin);
        tMax = std::min(tMax, axisMax);
        if (tMin > tMax) {
            return false;
        }
    }

    if (hitDistance) {
        *hitDistance = tMin;
    }

    return true;
}
