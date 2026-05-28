#include "platform/win32/DebugPanelPainter.h"

#include "platform/win32/AppState.h"
#include "platform/win32/DebugPanelContentPainter.h"
#include "platform/win32/Layout.h"
#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {
namespace {

void paintDebugMonitorChrome(
    HDC drawDc,
    AppDebugUiState& debugUiState,
    const int clientWidth,
    const int clientHeight,
    const int activeDebugMonitorHeight,
    const int debugMonitorTop,
    const int statusBarTop
)
{
    RECT debugPanelRect{0, debugMonitorTop, clientWidth, statusBarTop};
    UniqueBrush debugPanelBrush(CreateSolidBrush(RGB(21, 23, 28)));
    FillRect(drawDc, &debugPanelRect, debugPanelBrush.get());

    const RECT debugResizeRect =
        debugResizeRectForClient(activeDebugMonitorHeight, clientWidth, clientHeight);
    if (debugResizeRect.bottom > debugResizeRect.top) {
        UniqueBrush resizeBrush(CreateSolidBrush(
            debugUiState.debugResizeDragging ? RGB(50, 60, 76) : RGB(25, 28, 35)
        ));
        FillRect(drawDc, &debugResizeRect, resizeBrush.get());
    }

    UniquePen debugBorderPen(CreatePen(PS_SOLID, 1, RGB(62, 67, 78)));
    SelectObjectGuard penSelection(drawDc, debugBorderPen.get());
    MoveToEx(drawDc, 0, debugMonitorTop, nullptr);
    LineTo(drawDc, clientWidth, debugMonitorTop);
    if (debugResizeRect.bottom > debugResizeRect.top) {
        MoveToEx(drawDc, 0, debugResizeRect.bottom, nullptr);
        LineTo(drawDc, clientWidth, debugResizeRect.bottom);
    }
    MoveToEx(drawDc, 0, statusBarTop - 1, nullptr);
    LineTo(drawDc, clientWidth, statusBarTop - 1);
}

} // namespace

void paintDebugMonitorPanel(
    HDC drawDc,
    HFONT titleFont,
    HFONT bodyFont,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const int debugMonitorTop,
    const int statusBarTop
)
{
    if (statusBarTop <= debugMonitorTop) {
        return;
    }

    const int activeDebugMonitorHeight = statusBarTop - debugMonitorTop;
    paintDebugMonitorChrome(
        drawDc,
        static_cast<AppDebugUiState&>(state),
        clientWidth,
        clientHeight,
        activeDebugMonitorHeight,
        debugMonitorTop,
        statusBarTop
    );
    paintDebugMonitorContent(
        drawDc,
        titleFont,
        bodyFont,
        static_cast<const AppRuntimeState&>(state),
        static_cast<const AppRecordingState&>(state),
        static_cast<const AppOriginState&>(state),
        static_cast<const AppImportedSceneState&>(state),
        static_cast<const AppViewportState&>(state),
        static_cast<AppDebugUiState&>(state),
        clientWidth,
        clientHeight,
        activeDebugMonitorHeight,
        debugMonitorTop
    );
}

} // namespace ovtr::win32
