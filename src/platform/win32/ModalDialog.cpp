#include "platform/win32/ModalDialog.h"

namespace ovtr::win32 {

ModalDialogHost::ModalDialogHost(HWND parent) noexcept
    : parent_(parent)
{
    if (parent_) {
        EnableWindow(parent_, FALSE);
        parentDisabled_ = true;
    }
}

ModalDialogHost::~ModalDialogHost()
{
    restoreParent();
}

HWND ModalDialogHost::create(const ModalDialogDescriptor& descriptor, void* createParam)
{
    RECT dialogRect{0, 0, descriptor.clientWidth, descriptor.clientHeight};
    AdjustWindowRectEx(&dialogRect, descriptor.style, FALSE, descriptor.exStyle);
    const int dialogWindowWidth = dialogRect.right - dialogRect.left;
    const int dialogWindowHeight = dialogRect.bottom - dialogRect.top;

    RECT parentRect{};
    if (parent_) {
        GetWindowRect(parent_, &parentRect);
    }
    const int x = parentRect.left + ((parentRect.right - parentRect.left) - dialogWindowWidth) / 2;
    const int y = parentRect.top + ((parentRect.bottom - parentRect.top) - dialogWindowHeight) / 2;

    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parent_, GWLP_HINSTANCE));
    HWND dialogWindow = CreateWindowExW(
        descriptor.exStyle,
        descriptor.className,
        descriptor.title,
        descriptor.style,
        x,
        y,
        dialogWindowWidth,
        dialogWindowHeight,
        parent_,
        nullptr,
        instance,
        createParam
    );
    if (!dialogWindow) {
        return nullptr;
    }
    if (descriptor.title) {
        SetWindowTextW(dialogWindow, descriptor.title);
    }

    ShowWindow(dialogWindow, SW_SHOW);
    UpdateWindow(dialogWindow);
    return dialogWindow;
}

BOOL ModalDialogHost::runMessageLoop(HWND dialogWindow, const bool& done)
{
    messageResult_ = TRUE;
    while (!done && (messageResult_ = GetMessageW(&message_, nullptr, 0, 0)) > 0) {
        if (!IsWindow(dialogWindow) || !IsDialogMessageW(dialogWindow, &message_)) {
            TranslateMessage(&message_);
            DispatchMessageW(&message_);
        }
    }
    return messageResult_;
}

void ModalDialogHost::restoreParent() noexcept
{
    if (!parentDisabled_) {
        return;
    }
    EnableWindow(parent_, TRUE);
    SetActiveWindow(parent_);
    parentDisabled_ = false;
}

void ModalDialogHost::repostQuitIfReceived() const
{
    if (messageResult_ == 0) {
        PostQuitMessage(static_cast<int>(message_.wParam));
    }
}

} // namespace ovtr::win32
