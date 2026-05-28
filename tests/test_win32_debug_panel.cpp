#include "TestCases.h"
#include "TestSupport.h"

#include "platform/win32/DebugPanel.h"

#include <ctime>
#include <string>
#include <vector>

namespace ovtr::test {

void testWin32DebugPanel()
{
    require(ovtr::win32::yesNo(true) == L"Yes", "debug yes text");
    require(ovtr::win32::yesNo(false) == L"No", "debug no text");
    require(
        ovtr::win32::recorderStateText(ovtr::RecorderState::Idle) == L"Idle",
        "idle recorder text"
    );
    require(
        ovtr::win32::recorderStateText(ovtr::RecorderState::Finalizing) == L"Finalizing",
        "finalizing recorder text"
    );

    std::tm time{};
    time.tm_hour = 3;
    time.tm_min = 4;
    time.tm_sec = 5;
    require(
        ovtr::win32::formatDebugLogEntry(time, L"hello") == L"[03:04:05] hello",
        "debug log entry format"
    );

    std::vector<std::wstring> lines;
    int scrollOffset = 10;
    ovtr::win32::appendDebugLogEntry(lines, scrollOffset, L"first", time, 2);
    time.tm_sec = 6;
    ovtr::win32::appendDebugLogEntry(lines, scrollOffset, L"second", time, 2);
    time.tm_sec = 7;
    ovtr::win32::appendDebugLogEntry(lines, scrollOffset, L"third", time, 2);

    require(lines.size() == 2, "debug log max line count");
    require(lines[0] == L"[03:04:06] second", "debug log removes oldest line");
    require(lines[1] == L"[03:04:07] third", "debug log appends newest line");
    require(scrollOffset == 1, "debug log preserves in-range scroll offset");
}

} // namespace ovtr::test
