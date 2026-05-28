#include "platform/win32/OriginDialogSessionInternal.h"

#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/WindowLayout.h"

namespace ovtr::win32 {

void renderOriginDialogPreview(HWND hwnd, OriginDialogState& dialog)
{
    if (!dialog.appState) {
        return;
    }

    applyOriginDialogValuesToState(*dialog.appState, dialog.workingValues);
    dialog.appState->originStatusMessage = dialog.appState->originEnabled
        ? "origin preview updated"
        : "origin preview disabled";

    if (dialog.appState->glWindow) {
        renderViewport(dialog.appState->glWindow);
    }
    if (dialog.parent) {
        invalidateStatusPanel(dialog.parent);
        InvalidateRect(dialog.parent, nullptr, FALSE);
    } else {
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

void previewOriginDialogFromControls(HWND hwnd, OriginDialogState& dialog)
{
    if (dialog.updatingControls) {
        return;
    }
    if (!readOriginDialogControls(hwnd, dialog, false)) {
        return;
    }
    renderOriginDialogPreview(hwnd, dialog);
}

void restoreOriginDialogSnapshot(HWND hwnd, OriginDialogState& dialog)
{
    if (!dialog.appState) {
        return;
    }

    applyOriginDialogValuesToState(*dialog.appState, dialog.originalValues);
    dialog.appState->originStatusMessage = "origin settings canceled";
    appendDebugLog(*dialog.appState, dialog.appState->originStatusMessage);

    if (dialog.appState->glWindow) {
        renderViewport(dialog.appState->glWindow);
    }
    if (dialog.parent) {
        invalidateStatusPanel(dialog.parent);
        InvalidateRect(dialog.parent, nullptr, FALSE);
    } else {
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

} // namespace ovtr::win32
