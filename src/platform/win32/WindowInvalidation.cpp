#include "platform/win32/WindowLayout.h"

#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {
namespace {

constexpr int kStatusBarHeight = 36;
constexpr int kSplitterWidth = 8;

} // namespace

void invalidateWindowLayout(HWND hwnd)
{
    RedrawWindow(
        hwnd,
        nullptr,
        nullptr,
        RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW
    );
}

void invalidateStatusPanel(HWND hwnd)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    AppWindowState* state = appStateForWindow(hwnd);
    const int leftPanelWidth = leftPanelWidthForClient(state, clientRect.right - clientRect.left);
    const int statusBarTop = clientRect.bottom > kStatusBarHeight
        ? clientRect.bottom - kStatusBarHeight
        : 0;
    const int debugMonitorHeight = activeDebugMonitorHeight(
        state,
        clientRect.bottom - clientRect.top
    );
    const int debugMonitorTop = statusBarTop > debugMonitorHeight
        ? statusBarTop - debugMonitorHeight
        : statusBarTop;
    RECT statusPanelRect{0, 0, leftPanelWidth + kSplitterWidth, debugMonitorTop};
    RECT statusBarRect{0, debugMonitorTop, clientRect.right, clientRect.bottom};
    InvalidateRect(hwnd, &statusPanelRect, FALSE);
    InvalidateRect(hwnd, &statusBarRect, FALSE);
    const ViewportControlLayout controls = viewportControlLayoutForClient(
        state,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    if (controls.valid) {
        InvalidateRect(hwnd, &controls.barRect, FALSE);
    }
}

} // namespace ovtr::win32
