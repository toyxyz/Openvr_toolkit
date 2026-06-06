#include "platform/win32/WindowLayout.h"

#include "platform/win32/AppState.h"
#include "platform/win32/MappingNameEditor.h"
#include "platform/win32/MappingOffsetPresetNameEditor.h"
#include "platform/win32/ProfileEditor.h"
#include "platform/win32/SessionEditor.h"
#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {

void updateOriginEditorLayout(HWND hwnd, AppWindowState& state)
{
    if (!state.originEditWindow || !IsWindow(state.originEditWindow)) {
        state.originEditWindow = nullptr;
        state.originEditOriginalProc = nullptr;
        return;
    }

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const RECT editRect = originEditorRectForClient(
        &state,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        ShowWindow(state.originEditWindow, SW_HIDE);
        return;
    }

    MoveWindow(
        state.originEditWindow,
        editRect.left,
        editRect.top,
        editRect.right - editRect.left,
        editRect.bottom - editRect.top,
        TRUE
    );
    ShowWindow(state.originEditWindow, SW_SHOW);
}

void layoutChildWindows(HWND hwnd)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state || !state->glWindow) {
        return;
    }

    RECT rect;
    GetClientRect(hwnd, &rect);
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    state->debugMonitorHeight = clampDebugMonitorHeightForClient(state->debugMonitorHeight, height);
    const RECT glRect = viewportRenderRectForClient(state, width, height);
    const int glWidth = glRect.right - glRect.left;
    const int glHeight = glRect.bottom - glRect.top;

    if (glWidth > 160 && glHeight > 160) {
        MoveWindow(state->glWindow, glRect.left, glRect.top, glWidth, glHeight, TRUE);
        ShowWindow(state->glWindow, SW_SHOW);
    } else {
        ShowWindow(state->glWindow, SW_HIDE);
    }

    updateOriginEditorLayout(hwnd, *state);
    updateSessionEditorLayout(hwnd, *state);
    updateProfileEditorLayout(hwnd, *state);
    updateMappingActorNameEditorLayout(hwnd, *state);
    updateMappingNameEditorLayout(hwnd, *state);
    updateMappingOffsetPresetNameEditorLayout(hwnd, *state);
}

} // namespace ovtr::win32
