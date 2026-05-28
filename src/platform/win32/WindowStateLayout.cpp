#include "platform/win32/WindowLayout.h"

namespace ovtr::win32 {

int leftPanelContentBottomForClient(const AppWindowState* state, const int clientHeight)
{
    return contentBottomForClient(activeDebugMonitorHeight(state, clientHeight), clientHeight);
}

} // namespace ovtr::win32
