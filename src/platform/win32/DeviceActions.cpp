#include "platform/win32/DeviceActions.h"

#include "platform/win32/AppState.h"
#include "platform/win32/AppConfig.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/Win32String.h"

namespace ovtr::win32 {

bool setDeviceCustomName(HWND hwnd, AppWindowState& state, const ovtr::DeviceDescriptor& device)
{
    const std::string key = deviceNameKeyForDevice(device);
    const std::wstring deviceLabel = widen(deviceDisplayName(device));
    const std::wstring initialName = widen(customNameForDevice(state, device));

    std::wstring nameText;
    if (!promptForDeviceName(hwnd, deviceLabel, initialName, nameText)) {
        appendDebugLog(state, L"Set name canceled");
        return false;
    }

    const std::string customName = trimAscii(narrow(trimWide(nameText)));
    if (customName.empty()) {
        state.deviceCustomNames.erase(key);
        appendDebugLog(state, L"Device name cleared: " + deviceLabel);
    } else {
        state.deviceCustomNames[key] = customName;
        appendDebugLog(state, L"Device name set: " + deviceLabel + L" -> " + widen(customName));
    }

    saveDeviceNameConfig(state);
    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

} // namespace ovtr::win32
