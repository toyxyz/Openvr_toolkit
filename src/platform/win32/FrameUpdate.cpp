#include "platform/win32/FrameUpdate.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/ImportedScenePlayback.h"
#include "platform/win32/PoseSamplingWorker.h"
#include "platform/win32/RecordingUiActions.h"
#include "platform/win32/SessionPlayback.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {

void refreshPoseAndViewport(HWND hwnd)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return;
    }

    updateDelayedRecordingStart(hwnd, *state);
    updateImportedScenePlayback(*state);
    updateLoadedSessionPlayback(*state);
    if (state->loadedSessionActive) {
        sampleLoadedSessionFrame(*state);
    } else {
        state->poses = copyLatestPoseSnapshot(*state);
    }

    if (state->glWindow) {
        renderViewport(state->glWindow);
    }
    if (state->importedSceneLoaded || state->loadedSessionActive) {
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        const ViewportControlLayout controls = viewportControlLayoutForClient(
            state,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top
        );
        if (controls.animationValid) {
            InvalidateRect(hwnd, &controls.animationBarRect, FALSE);
        }
    }
}

} // namespace ovtr::win32
