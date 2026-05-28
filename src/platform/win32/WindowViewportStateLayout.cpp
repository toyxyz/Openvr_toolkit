#include "platform/win32/WindowLayout.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

ViewportControlLayout viewportControlLayoutForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    ViewportControlLayout layout;
    if (!state || clientWidth <= 0 || clientHeight <= 0) {
        return layout;
    }

    const int leftPanelWidth = leftPanelWidthForClient(state, clientWidth);
    const int contentBottom = leftPanelContentBottomForClient(state, clientHeight);
    return viewportControlLayoutForClient(
        leftPanelWidth,
        contentBottom,
        state->importedSceneLoaded,
        clientWidth,
        clientHeight
    );
}

RECT viewportRenderRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    if (!state || clientWidth <= 0 || clientHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    const int leftPanelWidth = leftPanelWidthForClient(state, clientWidth);
    const int contentBottom = leftPanelContentBottomForClient(state, clientHeight);
    const ViewportControlLayout controls = viewportControlLayoutForClient(
        state,
        clientWidth,
        clientHeight
    );
    return viewportRenderRectForClient(leftPanelWidth, contentBottom, controls, clientWidth);
}

} // namespace ovtr::win32
