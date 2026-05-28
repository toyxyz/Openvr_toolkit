#include "platform/win32/WindowClickActionSections.h"

#include "platform/win32/AppState.h"
#include "platform/win32/TopMenus.h"
#include "platform/win32/WindowLayout.h"

namespace ovtr::win32 {

bool handleTopBarClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const RECT settingRect = topBarSettingRectForClient(clientWidth, clientHeight);
    if (PtInRect(&settingRect, point)) {
        showTopSettingsMenu(hwnd, state, settingRect);
        return true;
    }
    const RECT fileRect = topBarFileRectForClient(clientWidth, clientHeight);
    if (PtInRect(&fileRect, point)) {
        showTopFileMenu(hwnd, state, fileRect);
        return true;
    }
    return false;
}

} // namespace ovtr::win32
