#include "platform/win32/WindowKeyboardSections.h"

#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/FrameUpdate.h"
#include "platform/win32/OriginActions.h"
#include "platform/win32/WindowLayout.h"

namespace ovtr::win32 {

bool handleOriginKeyDown(HWND hwnd, AppWindowState* state, const WPARAM wparam)
{
    if (wparam == VK_TAB) {
        if (state) {
            selectNextOriginDeviceWithLog(*state);
            invalidateStatusPanel(hwnd);
        }
        return true;
    }
    if (wparam == 'O') {
        if (state) {
            ensureOriginSelectionForUi(*state);
            const ovtr::DeviceDescriptor* selected = selectedOriginDevice(*state);
            if (!selected) {
                state->originStatusMessage = "no device selected for origin";
                appendDebugLog(*state, state->originStatusMessage);
            } else {
                const ovtr::DeviceDescriptor selectedSnapshot = *selected;
                if (confirmSetOrigin(hwnd, *state, selectedSnapshot)) {
                    setOriginFromDevice(*state, selectedSnapshot);
                    refreshPoseAndViewport(hwnd);
                }
            }
            invalidateStatusPanel(hwnd);
        }
        return true;
    }
    if (wparam == 'C') {
        if (state) {
            clearOrigin(*state);
            refreshPoseAndViewport(hwnd);
            invalidateStatusPanel(hwnd);
        }
        return true;
    }
    return false;
}

} // namespace ovtr::win32
