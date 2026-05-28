#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <string>
#include <vector>

namespace ovtr::win32 {

struct AppDebugUiState;

struct DebugMessageContentMetrics {
    int totalLineCount = 0;
    int firstLineIndex = 0;
    int lastLineIndex = 0;
};

void paintDebugInfoContent(
    HDC drawDc,
    HFONT bodyFont,
    AppDebugUiState& state,
    const std::vector<std::wstring>& debugLines,
    const RECT& bodyRect
);

DebugMessageContentMetrics paintDebugMessagesContent(
    HDC drawDc,
    HFONT bodyFont,
    AppDebugUiState& state,
    const RECT& bodyRect
);

std::wstring debugMessagesTitle(const DebugMessageContentMetrics& metrics);

} // namespace ovtr::win32
