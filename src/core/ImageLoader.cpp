#include "ImageLoader.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <string>

#pragma comment(lib, "gdiplus")

using namespace Gdiplus;

static ULONG_PTR gdiPlusToken = 0;

static void InitializeGdiPlus() {
    if (gdiPlusToken == 0) {
        GdiplusStartupInput startupInput;
        GdiplusStartup(&gdiPlusToken, &startupInput, nullptr);
    }
}

bool LoadImageFromFile(const std::string& filePath, int& width, int& height, int& channels, std::vector<unsigned char>& pixels) {
    InitializeGdiPlus();

    std::wstring widePath(filePath.begin(), filePath.end());
    Bitmap bitmap(widePath.c_str());
    if (bitmap.GetLastStatus() != Ok) {
        return false;
    }

    width = static_cast<int>(bitmap.GetWidth());
    height = static_cast<int>(bitmap.GetHeight());
    channels = 4;

    BitmapData bmpData;
    Rect rect(0, 0, width, height);
    if (bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData) != Ok) {
        return false;
    }

    pixels.resize(width * height * channels);
    for (int row = 0; row < height; ++row) {
        const unsigned char* src = reinterpret_cast<unsigned char*>(bmpData.Scan0) + row * bmpData.Stride;
        unsigned char* dst = pixels.data() + row * width * channels;
        for (int col = 0; col < width; ++col) {
            const unsigned char b = src[col * 4 + 0];
            const unsigned char g = src[col * 4 + 1];
            const unsigned char r = src[col * 4 + 2];
            const unsigned char a = src[col * 4 + 3];
            dst[col * 4 + 0] = r;
            dst[col * 4 + 1] = g;
            dst[col * 4 + 2] = b;
            dst[col * 4 + 3] = a;
        }
    }

    bitmap.UnlockBits(&bmpData);
    return true;
}
#else
bool LoadImageFromFile(const std::string&, int&, int&, int&, std::vector<unsigned char>&) {
    return false;
}
#endif
