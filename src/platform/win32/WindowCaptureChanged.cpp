#include "platform/win32/WindowInput.h"

#include "platform/win32/AppState.h"
#include "platform/win32/WindowStateAccess.h"

#include <chrono>

namespace ovtr::win32 {

void handleMainWindowCaptureChanged(HWND hwnd, LPARAM lparam)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (state && state->importedSceneTimelineDragging && reinterpret_cast<HWND>(lparam) != hwnd) {
        state->importedSceneTimelineDragging = false;
        state->importedSceneLastUpdate = std::chrono::steady_clock::now();
        InvalidateRect(hwnd, nullptr, FALSE);
    }
    if (state && state->loadedSessionTimelineDragging && reinterpret_cast<HWND>(lparam) != hwnd) {
        state->loadedSessionTimelineDragging = false;
        state->loadedSessionLastUpdate = std::chrono::steady_clock::now();
        InvalidateRect(hwnd, nullptr, FALSE);
    }
    if (state && state->debugResizeDragging && reinterpret_cast<HWND>(lparam) != hwnd) {
        state->debugResizeDragging = false;
        InvalidateRect(hwnd, nullptr, FALSE);
    }
    if (state && state->splitterDragging && reinterpret_cast<HWND>(lparam) != hwnd) {
        state->splitterDragging = false;
        InvalidateRect(hwnd, nullptr, FALSE);
    }
    if (state && state->profileSplitterDragging && reinterpret_cast<HWND>(lparam) != hwnd) {
        state->profileSplitterDragging = false;
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

} // namespace ovtr::win32
