#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct ModalDialogDescriptor {
    const wchar_t* className = nullptr;
    const wchar_t* title = nullptr;
    int clientWidth = 0;
    int clientHeight = 0;
    DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    DWORD exStyle = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE;
};

class ModalDialogHost {
public:
    explicit ModalDialogHost(HWND parent) noexcept;
    ~ModalDialogHost();

    ModalDialogHost(const ModalDialogHost&) = delete;
    ModalDialogHost& operator=(const ModalDialogHost&) = delete;

    HWND create(const ModalDialogDescriptor& descriptor, void* createParam);
    BOOL runMessageLoop(HWND dialogWindow, const bool& done);
    void restoreParent() noexcept;
    void repostQuitIfReceived() const;

private:
    HWND parent_ = nullptr;
    MSG message_{};
    BOOL messageResult_ = TRUE;
    bool parentDisabled_ = false;
};

} // namespace ovtr::win32
