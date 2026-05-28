#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "platform/win32/Win32AppWindowInternal.h"

#include "platform/win32/AppConfig.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/PoseSamplingWorker.h"
#include "platform/win32/SessionEditor.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/WindowStateAccess.h"

#include <memory>
#include <mutex>

namespace ovtr::win32 {

void createViewportChild(HWND hwnd, HINSTANCE instance, AppWindowState& state)
{
    state.glWindow = CreateWindowExW(
        0,
        kViewportWindowClassName,
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        760,
        32,
        400,
        400,
        hwnd,
        nullptr,
        instance,
        &state
    );
    if (state.glWindow) {
        if (setupOpenGLForChild(state.glWindow, state)) {
            appendDebugLog(state, L"OpenGL viewport initialized");
        } else {
            appendDebugLog(state, L"OpenGL viewport initialization failed");
        }
    }
}

void loadAppConfiguration(AppWindowState& state)
{
    loadOriginConfig(state);
    loadDeviceNameConfig(state);
    loadViewportSettingsConfig(state);
    loadRecordSettingsConfig(state);
}

void destroyAppWindowState(HWND hwnd)
{
    AppWindowState* rawState = appStateForWindow(hwnd);
    std::unique_ptr<AppWindowState> state(rawState);
    if (state) {
        if (state->originEditWindow) {
            DestroyWindow(state->originEditWindow);
            state->originEditWindow = nullptr;
            state->originEditOriginalProc = nullptr;
        }
        if (state->sessionEditWindow) {
            closeSessionEditor(hwnd, *state);
        }
        stopPoseSamplingWorker(*state);
        {
            std::lock_guard<std::mutex> providerLock(state->providerMutex);
            state->provider.shutdown();
        }
        shutdownOpenGLForChild(*state);
    }
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
}

} // namespace ovtr::win32
