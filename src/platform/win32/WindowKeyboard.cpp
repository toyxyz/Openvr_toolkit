#include "platform/win32/WindowInput.h"

#include "platform/win32/AppState.h"
#include "platform/win32/WindowKeyboardSections.h"
#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {

bool handleMainWindowKeyDown(HWND hwnd, WPARAM wparam)
{
    AppWindowState* state = appStateForWindow(hwnd);
    return handleCameraKeyDown(hwnd, state, wparam) ||
        handleRefreshKeyDown(hwnd, wparam) ||
        handleTrackedDeviceVisibilityKeyDown(hwnd, state, wparam) ||
        handleDeviceLabelKeyDown(hwnd, state, wparam) ||
        handleQuadViewKeyDown(hwnd, state, wparam) ||
        handleMappingCalibrationKeyDown(hwnd, state, wparam) ||
        handleRecordingKeyDown(hwnd, wparam);
}

} // namespace ovtr::win32
