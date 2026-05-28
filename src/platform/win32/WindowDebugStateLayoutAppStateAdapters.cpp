#include "platform/win32/WindowLayout.h"

#include "platform/win32/AppState.h"
#include "platform/win32/StatusPanel.h"

namespace ovtr::win32 {

int activeDebugMonitorHeight(const AppWindowState* state, const int clientHeight)
{
    if (!state || !state->debugMonitorVisible) {
        return 0;
    }

    return clampDebugMonitorHeightForClient(state->debugMonitorHeight, clientHeight);
}

RECT debugMessagesRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    return debugMessagesRectForClient(activeDebugMonitorHeight(state, clientHeight), clientWidth, clientHeight);
}

RECT debugInfoRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    return debugInfoRectForClient(activeDebugMonitorHeight(state, clientHeight), clientWidth, clientHeight);
}

RECT debugResizeRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    return debugResizeRectForClient(activeDebugMonitorHeight(state, clientHeight), clientWidth, clientHeight);
}

int maxDebugLogScrollOffset(const AppWindowState& state, const int visibleLineCount)
{
    return maxDebugScrollOffset(
        static_cast<int>(state.debugLogLines.size()),
        visibleLineCount
    );
}

void clampDebugLogScroll(AppWindowState& state, const int visibleLineCount)
{
    state.debugLogScrollOffset = clampDebugScrollOffset(
        state.debugLogScrollOffset,
        static_cast<int>(state.debugLogLines.size()),
        visibleLineCount
    );
}

int maxDebugInfoScrollOffset(const AppWindowState& state, const int visibleLineCount)
{
    const int totalLines = static_cast<int>(makeDebugMonitorLines(state).size()) - 1;
    return maxDebugScrollOffset(totalLines, visibleLineCount);
}

void clampDebugInfoScroll(AppWindowState& state, const int visibleLineCount)
{
    const int totalLines = static_cast<int>(makeDebugMonitorLines(state).size()) - 1;
    state.debugInfoScrollOffset = clampDebugScrollOffset(
        state.debugInfoScrollOffset,
        totalLines,
        visibleLineCount
    );
}

} // namespace ovtr::win32
