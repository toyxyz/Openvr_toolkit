#pragma once

#include "platform/win32/AppState.h"
#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {

inline void requestViewportRedraw(HWND hwnd)
{
    if (auto* state = appStateForWindow(hwnd)) state->viewportRenderDemand.dirty = true;
}

inline double viewportSceneSeconds(const AppWindowState& state) noexcept
{
    return state.importedSceneLoaded ? state.importedScenePlaybackSeconds : -1.0;
}

inline bool viewportHasAnimatedOverlay(const AppWindowState& state)
{
    std::lock_guard<std::mutex> lock(state.recordingMutex);
    return state.recordingDelayActive || state.recorder.state() == RecorderState::Recording;
}

inline bool viewportNeedsRedraw(HWND hwnd, const AppWindowState& state)
{
    return state.viewportRenderDemand.needsRender(IsWindowVisible(hwnd) && !IsIconic(hwnd),
        state.poses.timestampNs, state.poses.poses.size(), viewportSceneSeconds(state), viewportHasAnimatedOverlay(state));
}

inline void markViewportPresented(AppWindowState& state) noexcept
{
    state.viewportRenderDemand.presented(state.poses.timestampNs,
        state.poses.poses.size(), viewportSceneSeconds(state), viewportHasAnimatedOverlay(state));
}

} // namespace ovtr::win32
