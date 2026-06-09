#include "platform/win32/WindowChromePainter.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

void paintDeviceRailAndSplitter(
    HDC drawDc,
    HFONT font,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const int contentBottom
)
{
    if (state.sideMenusHiddenByShortcut) {
        return;
    }
    paintDeviceRailAndSplitter(
        drawDc,
        font,
        static_cast<const AppDebugUiState&>(state),
        clientWidth,
        clientHeight,
        contentBottom
    );
}

} // namespace ovtr::win32
