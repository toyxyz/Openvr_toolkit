#include "platform/win32/WindowLayout.h"

#include "platform/win32/AppState.h"
#include "platform/win32/RecordingSessionList.h"

namespace ovtr::win32 {

RECT sessionToggleButtonRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    if (!state || clientWidth <= 0 || clientHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    return sessionToggleButtonRectForClient(
        leftPanelContentBottomForClient(state, clientHeight),
        clientWidth,
        clientHeight
    );
}

RECT streamingToggleButtonRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    if (!state || clientWidth <= 0 || clientHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    return streamingToggleButtonRectForClient(
        leftPanelContentBottomForClient(state, clientHeight),
        clientWidth,
        clientHeight
    );
}

SessionListLayout sessionListLayoutForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight,
    const int sessionCount
)
{
    if (!state) {
        return {};
    }

    const OriginPanelLayout originLayout = originPanelLayoutForClient(state, clientWidth, clientHeight);
    const MarkerListLayout markerLayout = markerListLayoutForClient(state, clientWidth, clientHeight);
    bool lowerPanelValid = originLayout.valid;
    int lowerPanelTop = originLayout.boxRect.top;
    if (markerLayout.valid && (!lowerPanelValid || markerLayout.boxRect.top < lowerPanelTop)) {
        lowerPanelValid = true;
        lowerPanelTop = markerLayout.boxRect.top;
    }
    return sessionListLayoutForClient(
        state->sessionPanelVisible,
        leftPanelWidthForClient(state, clientWidth),
        leftPanelContentBottomForClient(state, clientHeight),
        lowerPanelValid,
        lowerPanelTop,
        sessionCount
    );
}

StreamingPanelLayout streamingPanelLayoutForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    if (!state) {
        return {};
    }
    return streamingPanelLayoutForClient(
        state->streamingPanelVisible,
        state->streamingOutputTarget == StreamingOutputTarget::Vmc,
        leftPanelWidthForClient(state, clientWidth),
        leftPanelContentBottomForClient(state, clientHeight)
    );
}

void clampSessionListScroll(
    AppWindowState& state,
    const int totalItemCount,
    const int visibleItemCount
)
{
    state.sessionListScrollOffset = clampSessionListScrollOffset(
        state.sessionListScrollOffset,
        totalItemCount,
        visibleItemCount
    );
}

} // namespace ovtr::win32
