#pragma once

#include "platform/win32/LayoutTypes.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct RecordingSessionListRow {
    std::wstring name;
    std::filesystem::path folder;
};

std::filesystem::path recordingSessionsRootPath();
std::vector<RecordingSessionListRow> listRecordingSessionFolders(
    const std::filesystem::path& recordingsRoot
);
int maxSessionListScrollOffset(int totalItemCount, int visibleItemCount) noexcept;
int clampSessionListScrollOffset(
    int scrollOffset,
    int totalItemCount,
    int visibleItemCount
) noexcept;
int sessionListItemTextRight(const SessionListLayout& layout, int totalItemCount) noexcept;
int sessionListRowIndexFromPoint(
    const SessionListLayout& layout,
    POINT point,
    int totalItemCount,
    int scrollOffset
) noexcept;

} // namespace ovtr::win32
