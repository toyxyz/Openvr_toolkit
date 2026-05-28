#include "platform/win32/WindowChromePainter.h"

#include "platform/win32/AppDebugUiState.h"
#include "platform/win32/Layout.h"
#include "platform/win32/PaintWidgets.h"
#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {
namespace {

constexpr int kTopBarHeight = 32;
constexpr int kDeviceToggleRailWidth = 32;
constexpr int kSplitterWidth = 8;

int activeDebugMonitorHeightForState(const AppDebugUiState& state, const int clientHeight)
{
    if (!state.debugMonitorVisible) {
        return 0;
    }
    return clampDebugMonitorHeightForClient(state.debugMonitorHeight, clientHeight);
}

int leftPanelWidthForState(const AppDebugUiState& state, const int clientWidth)
{
    const int requestedWidth = state.leftPanelWidth > 0 ? state.leftPanelWidth : 0;
    return leftPanelWidthForClient(state.devicePanelVisible, requestedWidth, clientWidth);
}

} // namespace

void paintDeviceRailAndSplitter(
    HDC drawDc,
    HFONT font,
    const AppDebugUiState& state,
    const int clientWidth,
    const int clientHeight,
    const int contentBottom
)
{
    RECT railRect{0, kTopBarHeight, kDeviceToggleRailWidth, contentBottom};
    UniqueBrush railBrush(CreateSolidBrush(RGB(20, 23, 28)));
    FillRect(drawDc, &railRect, railBrush.get());

    UniquePen railPen(CreatePen(PS_SOLID, 1, RGB(43, 48, 59)));
    {
        SelectObjectGuard penSelection(drawDc, railPen.get());
        MoveToEx(drawDc, kDeviceToggleRailWidth - 1, railRect.top, nullptr);
        LineTo(drawDc, kDeviceToggleRailWidth - 1, railRect.bottom);
    }

    const RECT deviceButtonRect = deviceToggleButtonRectForClient(contentBottom, clientWidth, clientHeight);
    if (deviceButtonRect.right > deviceButtonRect.left && deviceButtonRect.bottom > deviceButtonRect.top) {
        drawDeviceToggleButton(
            drawDc,
            font,
            deviceButtonRect,
            state.devicePanelVisible
        );
    }

    const RECT splitterRect = splitterRectForClient(
        leftPanelWidthForState(state, clientWidth),
        activeDebugMonitorHeightForState(state, clientHeight),
        clientHeight
    );
    if (splitterRect.bottom <= splitterRect.top || splitterRect.right <= splitterRect.left) {
        return;
    }

    UniqueBrush splitterBrush(CreateSolidBrush(
        state.splitterDragging ? RGB(50, 60, 76) : RGB(28, 31, 38)
    ));
    FillRect(drawDc, &splitterRect, splitterBrush.get());

    UniquePen splitterPen(CreatePen(PS_SOLID, 1, RGB(58, 64, 76)));
    {
        SelectObjectGuard penSelection(drawDc, splitterPen.get());
        MoveToEx(drawDc, splitterRect.left, splitterRect.top, nullptr);
        LineTo(drawDc, splitterRect.left, splitterRect.bottom);
        MoveToEx(drawDc, splitterRect.right - 1, splitterRect.top, nullptr);
        LineTo(drawDc, splitterRect.right - 1, splitterRect.bottom);
    }

    if (!state.devicePanelVisible) {
        return;
    }

    UniquePen handlePen(CreatePen(
        PS_SOLID,
        1,
        state.splitterDragging ? RGB(128, 154, 190) : RGB(84, 92, 108)
    ));
    SelectObjectGuard penSelection(drawDc, handlePen.get());
    const int handleX = splitterRect.left + (kSplitterWidth / 2);
    const int handleTop = splitterRect.top + 20;
    const int handleBottom = splitterRect.bottom > 20 ? splitterRect.bottom - 20 : splitterRect.bottom;
    MoveToEx(drawDc, handleX, handleTop, nullptr);
    LineTo(drawDc, handleX, handleBottom);
}

} // namespace ovtr::win32
