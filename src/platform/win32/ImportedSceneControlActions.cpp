#include "platform/win32/ImportedSceneActions.h"
#include "platform/win32/CombinedPlayback.h"

#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/ImportedScenePlayback.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/WindowLayout.h"

namespace ovtr::win32 {

void invalidateImportedAnimationBar(HWND hwnd, const AppWindowState& state)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const ViewportControlLayout layout = viewportControlLayoutForClient(
        &state,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    if (layout.animationValid) {
        InvalidateRect(hwnd, &layout.animationBarRect, FALSE);
    }
}

void seekImportedGlbFromTimeline(
    HWND hwnd,
    AppWindowState& state,
    const RECT& timelineRect,
    const POINT point
)
{
    if (!seekCombinedPlaybackFromTimeline(state, timelineRect, point)) {
        return;
    }

    invalidateImportedAnimationBar(hwnd, state);
}

void toggleImportedGlbPlayback(HWND hwnd, AppWindowState& state)
{
    if (!state.importedSceneLoaded) {
        return;
    }
    toggleCombinedPlayback(state);

    appendDebugLog(
        state,
        state.playbackTimeline.playing
            ? L"Imported GLB playback started"
            : L"Imported GLB playback paused"
    );
    invalidateStatusPanel(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void startImportedGlbPlaybackForRecording(HWND hwnd, AppWindowState& state)
{
    if (!state.importedSceneLoaded || combinedPlaybackDuration(state) <= 0.0) {
        invalidateImportedAnimationBar(hwnd, state);
        return;
    }
    if (state.playbackTimeline.seconds >= combinedPlaybackDuration(state)) seekCombinedPlayback(state, 0.0);
    state.playbackTimeline.playing = true;
    state.playbackTimeline.lastUpdate = PlaybackTimeline::Clock::now();
    applyCombinedPlayback(state, false, state.playbackTimeline.lastUpdate);
    appendDebugLog(state, L"Imported GLB playback auto-started for recording");
    invalidateImportedAnimationBar(hwnd, state);
}

bool handleImportedAnimationControlClick(
    HWND hwnd,
    AppWindowState& state,
    const ViewportControlLayout& layout,
    const POINT point
)
{
    if (!state.importedSceneLoaded || state.loadedSessionActive || !layout.animationValid) {
        return false;
    }

    if (PtInRect(&layout.firstFrameButtonRect, point)) {
        state.playbackTimeline.playing = false;
        seekCombinedPlayback(state, 0.0);
    } else if (PtInRect(&layout.playPauseButtonRect, point)) {
        toggleImportedGlbPlayback(hwnd, state);
        return true;
    } else if (PtInRect(&layout.lastFrameButtonRect, point)) {
        state.playbackTimeline.playing = false;
        seekCombinedPlayback(state, combinedPlaybackDuration(state));
    } else if (PtInRect(&layout.timelineRect, point)) {
        state.importedSceneTimelineDragging = true;
        SetCapture(hwnd);
        seekImportedGlbFromTimeline(hwnd, state, layout.timelineRect, point);
        return true;
    } else {
        return false;
    }

    state.viewportRenderDemand.dirty = true;
    invalidateStatusPanel(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

} // namespace ovtr::win32
