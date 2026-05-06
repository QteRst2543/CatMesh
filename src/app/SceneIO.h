#pragma once

#include <string>

class SceneDocument;

class SceneIO {
public:
    static bool LoadScene(const std::string& path, SceneDocument& document);
    static bool SaveScene(const std::string& path, const SceneDocument& document);
    static bool ExportSTL(const std::string& path, const SceneDocument& document);
};
