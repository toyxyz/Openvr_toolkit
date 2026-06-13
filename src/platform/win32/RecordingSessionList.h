#pragma once

#include "platform/win32/LayoutTypes.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct AppDebugUiState;

struct RecordingSessionListRow {
    std::wstring name;
    std::filesystem::path folder;
    std::wstring displayName;
    std::wstring createdLabel;
    std::uint64_t frameCount = 0;
    bool frameCountKnown = false;
};

inline constexpr int kSessionListItemHeight = 42;

std::filesystem::path recordingSessionsRootPath();
std::vector<RecordingSessionListRow> listRecordingSessionFolders(
    const std::filesystem::path& recordingsRoot
);
const std::vector<RecordingSessionListRow>& cachedRecordingSessionFolders(
    AppDebugUiState& state,
    const std::filesystem::path& recordingsRoot
);
void invalidateRecordingSessionListCache(AppDebugUiState& state);
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
