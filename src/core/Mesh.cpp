#include "Mesh.h"
#include "ImageLoader.h"
#include "core/Shader.h"
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

constexpr int kRenderStride = 8;

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
    if (textureId) glDeleteTextures(1, &textureId);
    if (shaderProgram) glDeleteProgram(shaderProgram);
    DestroyWireframe();
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
    texCoords.assign(editableVertices.size(), glm::vec2(0.0f));
    selectedFace = -1;
    selectedVertex = -1;
    RebuildWireframe();

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
            for (std::size_t vertexIndex = 0; vertexIndex < 3; ++vertexIndex) {
                const glm::vec3& vertex = triangleVertices[vertexIndex];
                const int sourceIndex = face[vertexIndex == 0 ? 0 : (vertexIndex == 1 ? i : i + 1)];
                const glm::vec2 uv = sourceIndex >= 0 && sourceIndex < static_cast<int>(texCoords.size())
                    ? texCoords[sourceIndex]
                    : glm::vec2(0.0f);

                vertices.push_back(vertex.x);
                vertices.push_back(vertex.y);
                vertices.push_back(vertex.z);
                vertices.push_back(normal.x);
                vertices.push_back(normal.y);
                vertices.push_back(normal.z);
                vertices.push_back(uv.x);
                vertices.push_back(uv.y);
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
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, kRenderStride * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

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
    float outerCutoffDegrees,
    const glm::mat4& lightSpaceMatrix,
    unsigned int shadowMap
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
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    // Передаем shadowMap в texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glUniform1i(glGetUniformLocation(shaderProgram, "shadowMap"), 0);
    
    // Исправление: используем texture unit 1 для текстуры объекта
    if (hasTexture && textureId) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUniform1i(glGetUniformLocation(shaderProgram, "textureSampler"), 1);
    }
    
    glUniform1i(glGetUniformLocation(shaderProgram, "hasTexture"), hasTexture ? 1 : 0);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    if (showWireframe && wireframeVAO && wireframeVertexCount > 0) {
        if (!wireframeShader) {
            const char* vertexShader = "#version 460 core\n"
                "layout (location = 0) in vec3 aPos;\n"
                "uniform mat4 model;\n"
                "uniform mat4 view;\n"
                "uniform mat4 projection;\n"
                "void main() {\n"
                "    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
                "}\n";
            const char* fragmentShader = "#version 460 core\n"
                "out vec4 FragColor;\n"
                "void main() {\n"
                "    FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
                "}\n";

            unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vs, 1, &vertexShader, nullptr);
            glCompileShader(vs);

            unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fs, 1, &fragmentShader, nullptr);
            glCompileShader(fs);

            wireframeShader = glCreateProgram();
            glAttachShader(wireframeShader, vs);
            glAttachShader(wireframeShader, fs);
            glLinkProgram(wireframeShader);

            glDeleteShader(vs);
            glDeleteShader(fs);
        }

        glUseProgram(wireframeShader);
        const glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        glUniformMatrix4fv(glGetUniformLocation(wireframeShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(wireframeShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(wireframeShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glLineWidth(1.5f);
        glBindVertexArray(wireframeVAO);
        size_t offset = 0;
        for (const auto& face : faces) {
            if (face.size() < 2) continue;
            glDrawArrays(GL_LINE_LOOP, static_cast<GLint>(offset), static_cast<GLint>(face.size()));
            offset += face.size();
        }
        glBindVertexArray(0);
    }
}

glm::vec3 Mesh::GetFaceNormal(int faceIndex) const {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(faces.size())) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }
    const auto& face = faces[faceIndex];
    if (face.size() < 3) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }
    const glm::vec3& p0 = editableVertices[face[0]];
    const glm::vec3& p1 = editableVertices[face[1]];
    const glm::vec3& p2 = editableVertices[face[2]];
    return SafeNormalize(glm::cross(p1 - p0, p2 - p0));
}

void Mesh::DrawSelectedFaceOutline(const glm::mat4& view, const glm::mat4& projection) const {
    if (editMode != EditMode::Face || selectedFace < 0 || selectedFace >= static_cast<int>(faces.size())) {
        return;
    }

    static unsigned int lineShader = 0;
    if (!lineShader) {
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
            "    FragColor = vec4(1.0, 0.9, 0.2, 1.0);\n"
            "}\n";

        unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertexShader, nullptr);
        glCompileShader(vs);

        unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragmentShader, nullptr);
        glCompileShader(fs);

        lineShader = glCreateProgram();
        glAttachShader(lineShader, vs);
        glAttachShader(lineShader, fs);
        glLinkProgram(lineShader);

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    const auto& face = faces[selectedFace];
    if (face.size() < 3) {
        return;
    }

    std::vector<float> lineVertices;
    lineVertices.reserve(face.size() * 3);
    for (int vertexIndex : face) {
        const glm::vec3 worldPos = editableVertices[vertexIndex] + position;
        lineVertices.push_back(worldPos.x);
        lineVertices.push_back(worldPos.y);
        lineVertices.push_back(worldPos.z);
    }

    unsigned int VAO = 0, VBO = 0;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(lineShader);
    glUniformMatrix4fv(glGetUniformLocation(lineShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(lineShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glLineWidth(3.0f);
    glDrawArrays(GL_LINE_LOOP, 0, static_cast<int>(face.size()));

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

void Mesh::SplitSelectedFace() {
    if (editMode != EditMode::Face || selectedFace < 0 || selectedFace >= static_cast<int>(faces.size())) {
        return;
    }

    const auto face = faces[selectedFace];
    if (face.size() < 3) {
        return;
    }

    glm::vec3 center(0.0f);
    for (int vertexIndex : face) {
        center += editableVertices[vertexIndex];
    }
    center /= static_cast<float>(face.size());

    int centerVertexIndex = static_cast<int>(editableVertices.size());
    editableVertices.push_back(center);

    std::vector<std::vector<int>> newFaces;
    newFaces.reserve(faces.size() + face.size() - 1);
    for (int i = 0; i < static_cast<int>(faces.size()); ++i) {
        if (i == selectedFace) {
            for (int j = 0; j < static_cast<int>(face.size()); ++j) {
                std::vector<int> triangle = {
                    face[j],
                    face[(j + 1) % face.size()],
                    centerVertexIndex
                };
                newFaces.push_back(std::move(triangle));
            }
        } else {
            newFaces.push_back(faces[i]);
        }
    }
    faces.swap(newFaces);
    RebuildFaceCenters();
    RebuildRenderData();
    RebuildWireframe();
    UploadBuffers();
    selectedFace = -1;
    selectedVertex = centerVertexIndex;
}

void Mesh::SplitSelectedVertex() {
    if (editMode != EditMode::Vertex || selectedVertex < 0 || selectedVertex >= static_cast<int>(editableVertices.size())) {
        return;
    }

    int faceIndex = -1;
    for (int i = 0; i < static_cast<int>(faces.size()); ++i) {
        const auto& face = faces[i];
        if (std::find(face.begin(), face.end(), selectedVertex) != face.end()) {
            faceIndex = i;
            break;
        }
    }
    if (faceIndex < 0) {
        return;
    }

    int newVertexIndex = static_cast<int>(editableVertices.size());
    editableVertices.push_back(editableVertices[selectedVertex]);

    auto& face = faces[faceIndex];
    for (int& vertexIndex : face) {
        if (vertexIndex == selectedVertex) {
            vertexIndex = newVertexIndex;
            break;
        }
    }

    RebuildFaceCenters();
    RebuildRenderData();
    RebuildWireframe();
    UploadBuffers();
    selectedVertex = newVertexIndex;
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
    RebuildWireframe();
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

void Mesh::TranslateByVector(const glm::vec3& delta) {
    position += delta;
}

bool Mesh::SetTextureFromFile(const std::string& path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<unsigned char> pixels;
    if (!LoadImageFromFile(path, width, height, channels, pixels)) {
        return false;
    }

    if (textureId) {
        glDeleteTextures(1, &textureId);
        textureId = 0;
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    hasTexture = true;
    return true;
}

void Mesh::ClearTexture() {
    if (textureId) {
        glDeleteTextures(1, &textureId);
        textureId = 0;
    }
    hasTexture = false;
}

bool Mesh::SetTexturedPlane(const std::string& path) {
    const std::vector<glm::vec3> planeVertices = {
        {-0.5f, -0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f},
        { 0.5f,  0.5f, 0.0f},
        {-0.5f,  0.5f, 0.0f}
    };

    const std::vector<std::vector<int>> planeFaces = {
        {0, 1, 2, 3}
    };

    const std::vector<glm::vec2> planeTexCoords = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    if (!SetGeometry(planeVertices, planeFaces)) {
        return false;
    }

    texCoords = planeTexCoords;
    RebuildRenderData();
    UploadBuffers();

    if (!SetTextureFromFile(path)) {
        return false;
    }

    position = glm::vec3(0.0f);
    color = glm::vec3(1.0f);
    return true;
}

void Mesh::ExtrudeSelectedAlongDirection(const glm::vec3& direction) {
    // Экструзия выбранной грани: сохраняем нижнюю грань, создаём верхнюю грань и боковые поверхности.
    if (editMode == EditMode::Face && selectedFace >= 0 && selectedFace < static_cast<int>(faces.size())) {
        const auto baseFace = faces[selectedFace];
        if (baseFace.size() < 3) {
            return;
        }

        std::vector<int> extrudedFaceIndices;
        extrudedFaceIndices.reserve(baseFace.size());
        for (int vertexIndex : baseFace) {
            editableVertices.push_back(editableVertices[vertexIndex] + direction);
            if (vertexIndex >= 0 && vertexIndex < static_cast<int>(texCoords.size())) {
                texCoords.push_back(texCoords[vertexIndex]);
            } else {
                texCoords.push_back(glm::vec2(0.0f));
            }
            extrudedFaceIndices.push_back(static_cast<int>(editableVertices.size()) - 1);
        }

        std::vector<std::vector<int>> sideFaces;
        const int faceCount = static_cast<int>(baseFace.size());
        for (int i = 0; i < faceCount; ++i) {
            int current = baseFace[i];
            int next = baseFace[(i + 1) % faceCount];
            int nextExtruded = extrudedFaceIndices[(i + 1) % faceCount];
            int currentExtruded = extrudedFaceIndices[i];
            sideFaces.push_back({ current, next, nextExtruded, currentExtruded });
        }

        // Сохраняем нижнюю грань, добавляем верхнюю грань и боковые грани.
        const int newTopFaceIndex = static_cast<int>(faces.size());
        faces.push_back(extrudedFaceIndices);
        for (auto& sideFace : sideFaces) {
            faces.push_back(std::move(sideFace));
        }
        selectedFace = newTopFaceIndex;
    }
    else if (editMode == EditMode::Vertex && selectedVertex >= 0 && selectedVertex < static_cast<int>(editableVertices.size())) {
        editableVertices[selectedVertex] += direction;
    }
    else {
        return;
    }

    RebuildFaceCenters();
    RebuildRenderData();
    RebuildWireframe();
    UploadBuffers();
}

void Mesh::RotateSelected(const glm::vec3& axis, float angleRadians) {
    glm::vec3 center = position; // Default to object center
    if (editMode == EditMode::Face && selectedFace >= 0) {
        center = GetFaceCenter(selectedFace) + position;
    } else if (editMode == EditMode::Vertex && selectedVertex >= 0) {
        center = GetVertexPosition(selectedVertex) + position;
    }

    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angleRadians, axis);
    std::set<int> affectedVertices;

    if (editMode == EditMode::Face && selectedFace >= 0) {
        affectedVertices.insert(faces[selectedFace].begin(), faces[selectedFace].end());
    } else if (editMode == EditMode::Vertex && selectedVertex >= 0) {
        affectedVertices.insert(selectedVertex);
    } else {
        // Rotate entire object
        for (size_t i = 0; i < editableVertices.size(); ++i) {
            affectedVertices.insert(static_cast<int>(i));
        }
    }

    for (int idx : affectedVertices) {
        glm::vec4 v = glm::vec4(editableVertices[idx] - (center - position), 1.0f);
        editableVertices[idx] = glm::vec3(rotation * v) + (center - position);
    }

    RebuildFaceCenters();
    RebuildRenderData();
    RebuildWireframe();
    UploadBuffers();
}

void Mesh::RebuildWireframe() {
    DestroyWireframe();

    std::vector<float> lineVertices;
    for (const auto& face : faces) {
        if (face.size() < 2) continue;
        for (int vertexIndex : face) {
            const glm::vec3 worldPos = editableVertices[vertexIndex];
            lineVertices.push_back(worldPos.x);
            lineVertices.push_back(worldPos.y);
            lineVertices.push_back(worldPos.z);
        }
    }

    if (lineVertices.empty()) return;

    wireframeVertexCount = static_cast<int>(lineVertices.size() / 3);
    glGenVertexArrays(1, &wireframeVAO);
    glGenBuffers(1, &wireframeVBO);

    glBindVertexArray(wireframeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, wireframeVBO);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Mesh::DestroyWireframe() {
    if (wireframeVAO) {
        glDeleteVertexArrays(1, &wireframeVAO);
        wireframeVAO = 0;
    }
    if (wireframeVBO) {
        glDeleteBuffers(1, &wireframeVBO);
        wireframeVBO = 0;
    }
    if (wireframeShader) {
        glDeleteProgram(wireframeShader);
        wireframeShader = 0;
    }
    wireframeVertexCount = 0;
}
