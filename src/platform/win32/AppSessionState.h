#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <string>

namespace ovtr::win32 {

struct AppSessionState {
    std::wstring sessionName;
    HWND sessionEditWindow = nullptr;
    WNDPROC sessionEditOriginalProc = nullptr;
};

} // namespace ovtr::win32
