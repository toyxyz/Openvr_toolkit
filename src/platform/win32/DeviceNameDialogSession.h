#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <string>

namespace ovtr::win32 {

inline constexpr UINT_PTR kDeviceNameEditControlId = 3001;

struct DeviceNameDialogState {
    HWND editWindow = nullptr;
    std::wstring subjectLabel = L"Device";
    std::wstring deviceLabel;
    std::wstring initialName;
    std::wstring resultName;
    bool accepted = false;
    bool done = false;
};

void createDeviceNameDialogControls(HWND hwnd, DeviceNameDialogState& dialog);
void finishDeviceNameDialog(HWND hwnd, DeviceNameDialogState& dialog, bool accepted);

} // namespace ovtr::win32
