#include "platform/win32/WindowInput.h"

#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/WindowStateAccess.h"

#include <chrono>
#include <string>

namespace ovtr::win32 {

bool handleMainWindowLButtonUp(HWND hwnd)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (state && state->importedSceneTimelineDragging) {
        state->importedSceneTimelineDragging = false;
        state->importedSceneLastUpdate = std::chrono::steady_clock::now();
        if (GetCapture() == hwnd) {
            ReleaseCapture();
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }
    if (state && state->debugResizeDragging) {
        state->debugResizeDragging = false;
        if (GetCapture() == hwnd) {
            ReleaseCapture();
        }
        appendDebugLog(*state, L"Debug panel height: " + std::to_wstring(state->debugMonitorHeight) + L" px");
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }
    if (state && state->splitterDragging) {
        state->splitterDragging = false;
        if (GetCapture() == hwnd) {
            ReleaseCapture();
        }
        appendDebugLog(*state, L"Left panel width: " + std::to_wstring(state->leftPanelWidth) + L" px");
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }
    if (state && state->profileSplitterDragging) {
        state->profileSplitterDragging = false;
        if (GetCapture() == hwnd) {
            ReleaseCapture();
        }
        appendDebugLog(*state, L"Right panel width: " + std::to_wstring(state->profilePanelWidth) + L" px");
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }

    return false;
}

} // namespace ovtr::win32
