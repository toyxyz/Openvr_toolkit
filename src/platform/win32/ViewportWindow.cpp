#include "platform/win32/ViewportWindow.h"

#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/ViewportWindowInput.h"
#include "platform/win32/Win32ScopedDcResources.h"
#include "platform/win32/WindowInput.h"

namespace ovtr::win32 {

LRESULT CALLBACK viewportProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_NCCREATE: {
        const auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lparam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
        return TRUE;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        renderViewport(hwnd);
        return 0;
    case WM_PAINT: {
        PaintDc paint(hwnd);
        if (!paint) {
            return 0;
        }
        renderViewport(hwnd);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        handleViewportLeftButtonDown(hwnd, lparam);
        return 0;
    }
    case WM_LBUTTONUP: {
        handleViewportLeftButtonUp(hwnd);
        return 0;
    }
    case WM_MBUTTONDOWN: {
        handleViewportMiddleButtonDown(hwnd, lparam);
        return 0;
    }
    case WM_MBUTTONUP: {
        handleViewportMiddleButtonUp(hwnd);
        return 0;
    }
    case WM_MOUSEMOVE: {
        handleViewportMouseMove(hwnd, lparam);
        return 0;
    }
    case WM_MOUSEWHEEL: {
        handleViewportMouseWheel(hwnd, wparam);
        return 0;
    }
    case WM_KEYDOWN: {
        HWND parent = GetParent(hwnd);
        if (parent && handleMainWindowKeyDown(parent, wparam)) {
            return 0;
        }
        break;
    }
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace ovtr::win32
