#include "platform/win32/ImportedSceneActions.h"

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
    if (!seekImportedSceneFromTimeline(state, timelineRect, point)) {
        return;
    }

    invalidateImportedAnimationBar(hwnd, state);
}

void toggleImportedGlbPlayback(HWND hwnd, AppWindowState& state)
{
    const ImportedScenePlaybackToggleResult result = toggleImportedScenePlayback(state);
    if (result == ImportedScenePlaybackToggleResult::Ignored) {
        return;
    }

    appendDebugLog(
        state,
        result == ImportedScenePlaybackToggleResult::Started
            ? L"Imported GLB playback started"
            : L"Imported GLB playback paused"
    );
    invalidateStatusPanel(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void startImportedGlbPlaybackForRecording(HWND hwnd, AppWindowState& state)
{
    if (!startImportedScenePlaybackForRecording(state)) {
        invalidateImportedAnimationBar(hwnd, state);
        return;
    }

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
    if (!state.importedSceneLoaded || !layout.animationValid) {
        return false;
    }

    if (PtInRect(&layout.firstFrameButtonRect, point)) {
        state.importedScenePlaying = false;
        setImportedScenePlaybackSeconds(state, 0.0);
    } else if (PtInRect(&layout.playPauseButtonRect, point)) {
        toggleImportedGlbPlayback(hwnd, state);
        return true;
    } else if (PtInRect(&layout.lastFrameButtonRect, point)) {
        state.importedScenePlaying = false;
        setImportedScenePlaybackSeconds(state, importedSceneDurationSeconds(state));
    } else if (PtInRect(&layout.closeButtonRect, point)) {
        closeImportedGlb(hwnd, state);
        return true;
    } else if (PtInRect(&layout.timelineRect, point)) {
        state.importedSceneTimelineDragging = true;
        SetCapture(hwnd);
        seekImportedGlbFromTimeline(hwnd, state, layout.timelineRect, point);
        return true;
    } else {
        return false;
    }

    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    invalidateStatusPanel(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

} // namespace ovtr::win32
