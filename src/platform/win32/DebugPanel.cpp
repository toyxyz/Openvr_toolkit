#include "platform/win32/DebugPanel.h"

#include <iomanip>
#include <sstream>

namespace ovtr::win32 {

std::wstring yesNo(const bool value)
{
    return value ? L"Yes" : L"No";
}

std::wstring recorderStateText(const RecorderState state)
{
    switch (state) {
    case RecorderState::Idle:
        return L"Idle";
    case RecorderState::Starting:
        return L"Starting";
    case RecorderState::Recording:
        return L"Recording";
    case RecorderState::Paused:
        return L"Paused";
    case RecorderState::Stopping:
        return L"Stopping";
    case RecorderState::Finalizing:
        return L"Finalizing";
    case RecorderState::Error:
        return L"Error";
    }

    return L"Unknown";
}

std::wstring formatDebugLogEntry(const std::tm& localTime, const std::wstring& message)
{
    std::wostringstream stream;
    stream << L"["
           << std::setw(2) << std::setfill(L'0') << localTime.tm_hour << L":"
           << std::setw(2) << std::setfill(L'0') << localTime.tm_min << L":"
           << std::setw(2) << std::setfill(L'0') << localTime.tm_sec << L"] "
           << message;
    return stream.str();
}

void appendDebugLogEntry(
    std::vector<std::wstring>& lines,
    int& scrollOffset,
    const std::wstring& message,
    const std::tm& localTime,
    const std::size_t maxLineCount
)
{
    lines.push_back(formatDebugLogEntry(localTime, message));
    if (maxLineCount > 0) {
        while (lines.size() > maxLineCount) {
            lines.erase(lines.begin());
        }
    }
    if (scrollOffset > static_cast<int>(lines.size())) {
        scrollOffset = static_cast<int>(lines.size());
    }
}

} // namespace ovtr::win32
