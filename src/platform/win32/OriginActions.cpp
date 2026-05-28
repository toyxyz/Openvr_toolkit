#include "platform/win32/OriginActions.h"

#include "platform/win32/AppConfig.h"
#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/OriginState.h"
#include "platform/win32/Win32String.h"

namespace ovtr::win32 {

bool confirmSetOrigin(HWND hwnd, AppWindowState& state, const ovtr::DeviceDescriptor& selected)
{
    std::wstring message = L"Set this device position and Y rotation as the origin?\n\n";
    message += widen(deviceDisplayName(selected));
    message += L"\n\nPosition and Y rotation will be used. X and Z rotation will be set to 0.";

    const int result = MessageBoxW(
        hwnd,
        message.c_str(),
        L"Set to Origin",
        MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2
    );
    if (result == IDOK) {
        return true;
    }

    state.originStatusMessage = "set origin canceled";
    appendDebugLog(state, state.originStatusMessage);
    return false;
}

void clearMissingDeviceSelectionWithLog(AppWindowState& state)
{
    if (clearMissingDeviceSelection(state)) {
        appendDebugLog(state, L"Device selection cleared: device unavailable");
    }
}

void toggleListDeviceSelection(AppWindowState& state, const ovtr::DeviceDescriptor& device)
{
    const ListDeviceSelectionChange change = toggleListDeviceSelectionState(state, device);
    if (change == ListDeviceSelectionChange::Cleared) {
        appendDebugLog(state, L"Device selection cleared");
        return;
    }

    appendDebugLog(state, L"Device selected: " + widen(deviceDisplayName(device)));
}

void ensureOriginSelectionForUi(AppWindowState& state)
{
    ensureOriginSelection(state);
}

void selectNextOriginDeviceWithLog(AppWindowState& state)
{
    appendDebugLog(state, selectNextOriginDevice(state));
}

bool setOriginFromDevice(AppWindowState& state, const ovtr::DeviceDescriptor& selected)
{
    if (!setOriginFromDevicePose(state, selected)) {
        appendDebugLog(state, state.originStatusMessage);
        return false;
    }

    appendDebugLog(state, state.originStatusMessage);
    saveOriginConfig(state);
    return true;
}

void clearOrigin(AppWindowState& state)
{
    clearOriginState(state);
    appendDebugLog(state, state.originStatusMessage);
    saveOriginConfig(state);
}

} // namespace ovtr::win32
