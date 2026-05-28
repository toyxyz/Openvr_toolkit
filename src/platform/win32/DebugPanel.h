#pragma once

#include "recording/RecordingState.h"

#include <ctime>
#include <string>
#include <vector>

namespace ovtr::win32 {

std::wstring yesNo(bool value);
std::wstring recorderStateText(RecorderState state);
std::wstring formatDebugLogEntry(const std::tm& localTime, const std::wstring& message);
void appendDebugLogEntry(
    std::vector<std::wstring>& lines,
    int& scrollOffset,
    const std::wstring& message,
    const std::tm& localTime,
    std::size_t maxLineCount
);

} // namespace ovtr::win32
