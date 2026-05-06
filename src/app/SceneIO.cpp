#include "app/SceneIO.h"
#include "app/SceneDocument.h"
#include "core/Mesh.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <utility>
#include <vector>

bool SceneIO::LoadScene(const std::string& path, SceneDocument& document) {
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "Failed to open scene file: " << path << std::endl;
        return false;
    }

    std::string token;
    in >> token;
    if (token != "CATMESH_SCENE_V1") {
        std::cerr << "Unsupported scene format: " << path << std::endl;
        return false;
    }

    std::size_t meshCount = 0;
    in >> token >> meshCount;
    if (!in || token != "mesh_count") {
        std::cerr << "Scene header is malformed." << std::endl;
        return false;
    }

    std::vector<std::shared_ptr<Mesh>> loadedMeshes;
    loadedMeshes.reserve(meshCount);

    for (std::size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex) {
        in >> token;
        if (!in || token != "mesh") {
            std::cerr << "Expected mesh block in scene file." << std::endl;
            return false;
        }

        glm::vec3 position(0.0f);
        glm::vec3 color(0.5f, 0.5f, 0.5f);
        std::size_t vertexCount = 0;
        std::size_t faceCount = 0;
        std::vector<glm::vec3> editableVertices;
        std::vector<std::vector<int>> faces;

        in >> token >> position.x >> position.y >> position.z;
        if (!in || token != "position") {
            std::cerr << "Scene mesh position block is malformed." << std::endl;
            return false;
        }

        in >> token >> color.x >> color.y >> color.z;
        if (!in || token != "color") {
            std::cerr << "Scene mesh color block is malformed." << std::endl;
            return false;
        }

        in >> token >> vertexCount;
        if (!in || token != "vertex_count") {
            std::cerr << "Scene vertex block is malformed." << std::endl;
            return false;
        }

        editableVertices.reserve(vertexCount);
        for (std::size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
            glm::vec3 vertex(0.0f);
            in >> token >> vertex.x >> vertex.y >> vertex.z;
            if (!in || token != "v") {
                std::cerr << "Scene vertex entry is malformed." << std::endl;
                return false;
            }
            editableVertices.push_back(vertex);
        }

        in >> token >> faceCount;
        if (!in || token != "face_count") {
            std::cerr << "Scene face block is malformed." << std::endl;
            return false;
        }

        faces.reserve(faceCount);
        for (std::size_t faceIndex = 0; faceIndex < faceCount; ++faceIndex) {
            std::size_t indicesInFace = 0;
            in >> token >> indicesInFace;
            if (!in || token != "f" || indicesInFace < 3) {
                std::cerr << "Scene face entry is malformed." << std::endl;
                return false;
            }

            std::vector<int> face(indicesInFace, 0);
            for (std::size_t i = 0; i < indicesInFace; ++i) {
                in >> face[i];
            }

            if (!in) {
                std::cerr << "Failed to read face indices." << std::endl;
                return false;
            }

            faces.push_back(std::move(face));
        }

        in >> token;
        if (!in || token != "end_mesh") {
            std::cerr << "Scene mesh terminator is missing." << std::endl;
            return false;
        }

        auto mesh = std::make_shared<Mesh>();
        if (!mesh->SetGeometry(editableVertices, faces)) {
            return false;
        }

        mesh->SetPosition(position);
        mesh->SetColor(color);
        mesh->SetEditMode(Mesh::EditMode::Object);
        loadedMeshes.push_back(mesh);
    }

    document.ReplaceScene(std::move(loadedMeshes));
    return true;
}

bool SceneIO::SaveScene(const std::string& path, const SceneDocument& document) {
    std::ofstream out(path);
    if (!out.is_open()) {
        std::cerr << "Failed to open scene file for writing: " << path << std::endl;
        return false;
    }

    out << "CATMESH_SCENE_V1\n";
    out << "mesh_count " << document.GetMeshes().size() << "\n";

    for (const auto& mesh : document.GetMeshes()) {
        out << "mesh\n";

        const glm::vec3 position = mesh->GetPosition();
        out << "position " << position.x << " " << position.y << " " << position.z << "\n";

        const glm::vec3 color = mesh->GetColor();
        out << "color " << color.x << " " << color.y << " " << color.z << "\n";

        const auto& editableVertices = mesh->GetEditableVertices();
        out << "vertex_count " << editableVertices.size() << "\n";
        for (const glm::vec3& vertex : editableVertices) {
            out << "v " << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
        }

        const auto& faces = mesh->GetFaces();
        out << "face_count " << faces.size() << "\n";
        for (const auto& face : faces) {
            out << "f " << face.size();
            for (int index : face) {
                out << " " << index;
            }
            out << "\n";
        }

        out << "end_mesh\n";
    }

    return true;
}

bool SceneIO::ExportSTL(const std::string& path, const SceneDocument& document) {
    if (document.GetMeshes().empty()) {
        std::cout << "No meshes to export!" << std::endl;
        return false;
    }

    std::ofstream out(path);
    if (!out.is_open()) {
        std::cout << "Failed to open file for writing: " << path << std::endl;
        return false;
    }

    out << std::fixed << std::setprecision(6);

    std::size_t exportedFacetCount = 0;
    const auto& meshes = document.GetMeshes();
    for (std::size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
        const auto& mesh = meshes[meshIndex];
        const glm::vec3 meshPosition = mesh->GetPosition();
        const auto& editableVertices = mesh->GetEditableVertices();
        const auto& logicalFaces = mesh->GetFaces();
        out << "solid mesh_" << meshIndex << "\n";

        for (std::size_t faceIndex = 0; faceIndex < logicalFaces.size(); ++faceIndex) {
            const auto& face = logicalFaces[faceIndex];
            const glm::vec3 faceNormal = mesh->GetFaceNormal(static_cast<int>(faceIndex));
            for (std::size_t i = 1; i + 1 < face.size(); ++i) {
                const glm::vec3 v0 = editableVertices[face[0]] + meshPosition;
                glm::vec3 v1 = editableVertices[face[i]] + meshPosition;
                glm::vec3 v2 = editableVertices[face[i + 1]] + meshPosition;

                glm::vec3 triangleNormal = glm::cross(v1 - v0, v2 - v0);
                const float normalLength = glm::length(triangleNormal);
                if (normalLength <= 0.000001f) {
                    continue;
                }

                if (glm::dot(triangleNormal, faceNormal) < 0.0f) {
                    std::swap(v1, v2);
                    triangleNormal = -triangleNormal;
                }

                const glm::vec3 normal = triangleNormal / normalLength;

                out << "facet normal " << normal.x << " " << normal.y << " " << normal.z << "\n";
                out << "  outer loop\n";
                out << "    vertex " << v0.x << " " << v0.y << " " << v0.z << "\n";
                out << "    vertex " << v1.x << " " << v1.y << " " << v1.z << "\n";
                out << "    vertex " << v2.x << " " << v2.y << " " << v2.z << "\n";
                out << "  endloop\n";
                out << "endfacet\n";
                ++exportedFacetCount;
            }
        }

        out << "endsolid mesh_" << meshIndex << "\n";
    }

    std::cout << "Exported " << meshes.size() << " mesh(es) and "
              << exportedFacetCount << " facet(s) to: " << path << std::endl;
    return true;
}
