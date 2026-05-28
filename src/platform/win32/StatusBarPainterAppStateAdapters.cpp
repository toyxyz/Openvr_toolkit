#include "platform/win32/StatusBarPainter.h"

#include "platform/win32/AppState.h"
#include "platform/win32/StatusPanel.h"

namespace ovtr::win32 {

void paintStatusBar(
    HDC drawDc,
    HFONT font,
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight,
    const int statusBarTop
)
{
    if (!state) {
        paintStatusBar(
            drawDc,
            font,
            static_cast<const StatusBarPaintState*>(nullptr),
            clientWidth,
            clientHeight,
            statusBarTop
        );
        return;
    }

    const StatusBarPaintState paintState{
        makeStatusBarMessage(*state),
        makeStatusBarMetrics(*state),
        state->debugMonitorVisible,
    };
    paintStatusBar(drawDc, font, &paintState, clientWidth, clientHeight, statusBarTop);
}

} // namespace ovtr::win32
