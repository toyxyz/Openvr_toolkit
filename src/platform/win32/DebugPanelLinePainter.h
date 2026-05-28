#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <string>
#include <vector>

namespace ovtr::win32 {

void paintDebugInfoLines(
    HDC drawDc,
    HFONT bodyFont,
    const std::vector<std::wstring>& debugLines,
    const RECT& bodyRect,
    int textRight,
    int firstLineIndex,
    int lastLineIndex
);

void paintDebugMessageLines(
    HDC drawDc,
    HFONT bodyFont,
    const std::vector<std::wstring>& messageLines,
    const RECT& bodyRect,
    int textRight,
    int firstLineIndex,
    int lastLineIndex
);

} // namespace ovtr::win32
