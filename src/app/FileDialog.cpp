#include "app/FileDialog.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef APIENTRY
#undef APIENTRY
#endif
#include <windows.h>
#include <commdlg.h>
#endif

std::string ShowWindowsFileDialog(const char* filter, unsigned long flags, const char* defaultExtension) {
#ifdef _WIN32
    char fileName[MAX_PATH] = {};

    OPENFILENAMEA dialogConfig = {};
    dialogConfig.lStructSize = sizeof(dialogConfig);
    dialogConfig.hwndOwner = nullptr;
    dialogConfig.lpstrFile = fileName;
    dialogConfig.nMaxFile = MAX_PATH;
    dialogConfig.lpstrFilter = filter;
    dialogConfig.nFilterIndex = 1;
    dialogConfig.Flags = flags;
    dialogConfig.lpstrDefExt = defaultExtension;

    const BOOL result = (flags & OFN_FILEMUSTEXIST)
        ? GetOpenFileNameA(&dialogConfig)
        : GetSaveFileNameA(&dialogConfig);

    return result ? std::string(fileName) : std::string();
#else
    return std::string();
#endif
}
