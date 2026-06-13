#include "platform/win32/RecordingSessionList.h"

#include "platform/win32/AppDebugUiState.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/DeviceListLayoutMetrics.h"
#include "platform/win32/Layout.h"
#include "platform/win32/Win32String.h"
#include "recording/SessionManifest.h"

#include <algorithm>
#include <cwchar>
#include <cstdint>
#include <system_error>

namespace ovtr::win32 {
namespace {

bool looksLikeRecordingSessionFolder(const std::filesystem::path& folder)
{
    if (folder.filename().string().rfind("session_", 0) == 0) {
        return true;
    }
    std::error_code error;
    return std::filesystem::exists(folder / "manifest.json", error) && !error;
}

std::filesystem::file_time_type sessionFolderWriteTime(const std::filesystem::directory_entry& entry)
{
    std::error_code error;
    const std::filesystem::file_time_type writeTime = entry.last_write_time(error);
    return error ? (std::filesystem::file_time_type::min)() : writeTime;
}

std::uint64_t fileTimeTicks(const FILETIME& fileTime) noexcept
{
    ULARGE_INTEGER value{};
    value.LowPart = fileTime.dwLowDateTime;
    value.HighPart = fileTime.dwHighDateTime;
    return value.QuadPart;
}

std::uint64_t sessionFolderCreationTime(const std::filesystem::path& folder)
{
    WIN32_FILE_ATTRIBUTE_DATA attributes{};
    if (!GetFileAttributesExW(folder.wstring().c_str(), GetFileExInfoStandard, &attributes)) {
        return 0;
    }
    return fileTimeTicks(attributes.ftCreationTime);
}

std::wstring formatSystemTimeMinute(const SYSTEMTIME& time)
{
    wchar_t buffer[24]{};
    std::swprintf(
        buffer,
        24,
        L"%04u-%02u-%02u %02u:%02u",
        static_cast<unsigned>(time.wYear),
        static_cast<unsigned>(time.wMonth),
        static_cast<unsigned>(time.wDay),
        static_cast<unsigned>(time.wHour),
        static_cast<unsigned>(time.wMinute)
    );
    return buffer;
}

std::wstring formatFolderCreationLabel(const std::filesystem::path& folder)
{
    WIN32_FILE_ATTRIBUTE_DATA attributes{};
    if (!GetFileAttributesExW(folder.wstring().c_str(), GetFileExInfoStandard, &attributes)) {
        return {};
    }
    FILETIME localTime{};
    SYSTEMTIME systemTime{};
    if (!FileTimeToLocalFileTime(&attributes.ftCreationTime, &localTime) ||
        !FileTimeToSystemTime(&localTime, &systemTime)) {
        return {};
    }
    return formatSystemTimeMinute(systemTime);
}

std::wstring formatCreatedAtUtcLabel(const std::string& value)
{
    if (value.size() >= 16 && value[4] == '-' && value[7] == '-' &&
        (value[10] == 'T' || value[10] == ' ') && value[13] == ':') {
        return widen(value.substr(0, 10) + " " + value.substr(11, 5));
    }
    return widen(value);
}

void enrichSessionListRow(const std::filesystem::path& folder, RecordingSessionListRow& row)
{
    RecordingSession session;
    SessionManifestStats stats;
    std::string error;
    if (readManifestJson(folder / "manifest.json", session, stats, error)) {
        if (!session.sessionName.empty()) {
            row.displayName = widen(session.sessionName);
        }
        if (!session.createdAtUtc.empty()) {
            row.createdLabel = formatCreatedAtUtcLabel(session.createdAtUtc);
        }
        if (stats.frameCount > 0 || stats.finalized || stats.durationSeconds > 0.0) {
            row.frameCount = stats.frameCount;
            row.frameCountKnown = true;
        }
    }
}

RecordingSessionListRow makeSessionListRow(const std::filesystem::directory_entry& entry)
{
    const std::string name = entry.path().filename().string();
    RecordingSessionListRow row{widen(name), entry.path()};
    row.displayName = row.name;
    row.createdLabel = formatFolderCreationLabel(entry.path());
    enrichSessionListRow(entry.path(), row);
    return row;
}

struct SessionFolderListCandidate {
    RecordingSessionListRow row;
    std::uint64_t creationTime = 0;
    std::filesystem::file_time_type writeTime{};
};

} // namespace

std::filesystem::path recordingSessionsRootPath()
{
    return defaultSessionDirectoryPath();
}

std::vector<RecordingSessionListRow> listRecordingSessionFolders(
    const std::filesystem::path& recordingsRoot
)
{
    std::vector<SessionFolderListCandidate> candidates;
    std::error_code error;
    if (!std::filesystem::exists(recordingsRoot, error) ||
        !std::filesystem::is_directory(recordingsRoot, error)) {
        return {};
    }

    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(recordingsRoot, error)) {
        if (error) {
            break;
        }
        if (!entry.is_directory(error)) {
            continue;
        }
        if (!looksLikeRecordingSessionFolder(entry.path())) {
            continue;
        }
        candidates.push_back(SessionFolderListCandidate{
            makeSessionListRow(entry),
            sessionFolderCreationTime(entry.path()),
            sessionFolderWriteTime(entry)
        });
    }

    std::sort(candidates.begin(), candidates.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.creationTime != rhs.creationTime) {
            return lhs.creationTime > rhs.creationTime;
        }
        if (lhs.writeTime != rhs.writeTime) {
            return lhs.writeTime > rhs.writeTime;
        }
        return lhs.row.name > rhs.row.name;
    });
    std::vector<RecordingSessionListRow> rows;
    rows.reserve(candidates.size());
    for (const SessionFolderListCandidate& candidate : candidates) {
        rows.push_back(candidate.row);
    }
    return rows;
}

const std::vector<RecordingSessionListRow>& cachedRecordingSessionFolders(
    AppDebugUiState& state,
    const std::filesystem::path& recordingsRoot
) {
    const std::filesystem::path normalizedRoot = recordingsRoot.lexically_normal();
    if (!state.sessionListCacheValid || state.sessionListCacheRoot != normalizedRoot) {
        state.sessionListCacheRows = listRecordingSessionFolders(recordingsRoot);
        state.sessionListCacheRoot = normalizedRoot;
        state.sessionListCacheValid = true;
    }
    return state.sessionListCacheRows;
}

void invalidateRecordingSessionListCache(AppDebugUiState& state)
{
    state.sessionListCacheValid = false;
    state.sessionListCacheRows.clear();
}

int maxSessionListScrollOffset(const int totalItemCount, const int visibleItemCount) noexcept
{
    return maxDeviceListScrollOffset(totalItemCount, visibleItemCount);
}

int clampSessionListScrollOffset(
    const int scrollOffset,
    const int totalItemCount,
    const int visibleItemCount
) noexcept
{
    return clampDeviceListScrollOffset(scrollOffset, totalItemCount, visibleItemCount);
}

int sessionListItemTextRight(const SessionListLayout& layout, const int totalItemCount) noexcept
{
    if (!layout.valid) {
        return 0;
    }
    return maxSessionListScrollOffset(totalItemCount, layout.visibleItemCount) > 0
        ? layout.contentRect.right - 14
        : layout.contentRect.right;
}

int sessionListRowIndexFromPoint(
    const SessionListLayout& layout,
    const POINT point,
    const int totalItemCount,
    const int scrollOffset
) noexcept
{
    if (!layout.valid || totalItemCount <= 0 || !PtInRect(&layout.contentRect, point)) {
        return -1;
    }
    if (point.x >= sessionListItemTextRight(layout, totalItemCount)) {
        return -1;
    }
    const int visibleRowIndex = (point.y - layout.contentRect.top) / kSessionListItemHeight;
    if (visibleRowIndex < 0 || visibleRowIndex >= layout.visibleItemCount) {
        return -1;
    }
    const int rowIndex = scrollOffset + visibleRowIndex;
    return rowIndex >= 0 && rowIndex < totalItemCount ? rowIndex : -1;
}

} // namespace ovtr::win32
