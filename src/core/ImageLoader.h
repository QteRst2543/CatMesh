#pragma once

#include <string>
#include <vector>

bool LoadImageFromFile(const std::string& filePath, int& width, int& height, int& channels, std::vector<unsigned char>& pixels);
