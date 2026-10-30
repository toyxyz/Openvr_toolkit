#include "platform/win32/WindowLayout.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

RECT videoToggleButtonRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
) {
    if (!state) {
        return RECT{0, 0, 0, 0};
    }
    return videoToggleButtonRectForClient(
        leftPanelContentBottomForClient(state, clientHeight), clientWidth, clientHeight);
}

VideoPanelLayout videoPanelLayoutForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
) {
    if (!state) {
        return {};
    }
    return videoPanelLayoutForClient(
        state->videoPanelVisible,
        leftPanelWidthForClient(state, clientWidth),
        leftPanelContentBottomForClient(state, clientHeight),
        static_cast<int>(state->videoCameras.size()) + 1);
}

} // namespace ovtr::win32
