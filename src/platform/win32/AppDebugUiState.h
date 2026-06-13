#pragma once

#include "platform/win32/RecordingSessionList.h"

#include <filesystem>
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
    bool sessionPanelVisible = false;
    bool streamingPanelVisible = false;
    int sessionListScrollOffset = 0;
    std::wstring selectedSessionName;
    std::filesystem::path sessionListCacheRoot;
    std::vector<RecordingSessionListRow> sessionListCacheRows;
    bool sessionListCacheValid = false;
    bool splitterDragging = false;
    bool debugResizeDragging = false;
};

} // namespace ovtr::win32
