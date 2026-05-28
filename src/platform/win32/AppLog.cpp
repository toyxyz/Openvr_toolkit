#include "platform/win32/AppLog.h"

#include "platform/win32/AppDebugUiState.h"
#include "platform/win32/DebugPanel.h"
#include "platform/win32/Win32String.h"

#include <ctime>

namespace ovtr::win32 {
namespace {

constexpr std::size_t kDebugLogMaxLines = 80;

} // namespace

void appendDebugLog(AppDebugUiState& state, const std::wstring& message)
{
    const std::time_t currentTime = std::time(nullptr);
    std::tm localTime{};
    localtime_s(&localTime, &currentTime);

    appendDebugLogEntry(
        state.debugLogLines,
        state.debugLogScrollOffset,
        message,
        localTime,
        kDebugLogMaxLines
    );
}

void appendDebugLog(AppDebugUiState& state, const std::string& message)
{
    appendDebugLog(state, widen(message));
}

} // namespace ovtr::win32
