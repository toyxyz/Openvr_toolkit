#pragma once

#include <string>
#include <vector>

namespace ovtr::win32 {

struct AppDebugUiState {
    bool debugMonitorVisible = false;
    int debugLogScrollOffset = 0;
    int debugInfoScrollOffset = 0;
    int debugMonitorHeight = 220;
    std::vector<std::wstring> debugLogLines;
    int leftPanelWidth = 0;
    bool devicePanelVisible = true;
    bool splitterDragging = false;
    bool debugResizeDragging = false;
};

} // namespace ovtr::win32
