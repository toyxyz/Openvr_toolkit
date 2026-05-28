#include "platform/win32/WindowDeviceContextMenuActions.h"

#include "platform/win32/AppState.h"
#include "platform/win32/DeviceActions.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/FrameUpdate.h"
#include "platform/win32/OriginActions.h"
#include "platform/win32/PoseSamplingWorker.h"
#include "platform/win32/WindowDeviceContextMenuIds.h"
#include "platform/win32/WindowLayout.h"

namespace ovtr::win32 {

bool executeDeviceContextMenuCommand(HWND hwnd, AppWindowState& state, const UINT command)
{
    if (command == kDeviceContextMenuSetNameId) {
        const ovtr::DeviceDescriptor* selected = selectedListDevice(state);
        if (selected) {
            setDeviceCustomName(hwnd, state, *selected);
        }
        invalidateStatusPanel(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }

    if (command == kDeviceContextMenuSetOriginId) {
        const ovtr::DeviceDescriptor* selected = selectedListDevice(state);
        if (!selected) {
            invalidateStatusPanel(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
            return true;
        }

        const ovtr::DeviceDescriptor selectedSnapshot = *selected;
        if (!confirmSetOrigin(hwnd, state, selectedSnapshot)) {
            invalidateStatusPanel(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
            return true;
        }
        state.poses = copyLatestPoseSnapshot(state);
        if (setOriginFromDevice(state, selectedSnapshot)) {
            refreshPoseAndViewport(hwnd);
        }
        invalidateStatusPanel(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }

    return false;
}

} // namespace ovtr::win32
