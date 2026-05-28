#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/FrameUpdate.h"
#include "platform/win32/Menus.h"
#include "platform/win32/PoseSamplingWorker.h"
#include "platform/win32/RuntimeStatus.h"
#include "platform/win32/Win32AppWindowMessages.h"
#include "platform/win32/WindowInput.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowPainter.h"

namespace ovtr::win32 {
namespace {

constexpr UINT_PTR kStatusTimerId = 1;
constexpr UINT kStatusIntervalMs = 1000;

} // namespace

LRESULT CALLBACK mainWindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_MEASUREITEM: {
        auto* measure = reinterpret_cast<MEASUREITEMSTRUCT*>(lparam);
        if (measure && measurePopupMenuItem(hwnd, *measure)) {
            return TRUE;
        }
        break;
    }
    case WM_DRAWITEM: {
        const auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lparam);
        if (draw && drawPopupMenuItem(*draw)) {
            return TRUE;
        }
        break;
    }
    case WM_CREATE: {
        SetTimer(hwnd, kStatusTimerId, kStatusIntervalMs, nullptr);
        return handleMainWindowCreate(hwnd, lparam) ? 0 : -1;
    }
    case WM_DESTROY: {
        handleMainWindowDestroy(hwnd, kStatusTimerId);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_TIMER:
        if (wparam == kStatusTimerId) {
            refreshStatus(hwnd);
            return 0;
        }
        break;
    case kPoseSamplingStatusMessage:
        invalidateStatusPanel(hwnd);
        return 0;
    case WM_SIZE:
        layoutChildWindows(hwnd);
        invalidateWindowLayout(hwnd);
        return 0;
    case WM_SETCURSOR:
        if (handleMainWindowSetCursor(hwnd, lparam)) {
            return TRUE;
        }
        break;
    case WM_MOUSEWHEEL:
        if (handleMainWindowMouseWheel(hwnd, wparam, lparam)) {
            return 0;
        }
        break;
    case WM_MOUSEMOVE:
        if (handleMainWindowMouseMove(hwnd, lparam)) {
            return 0;
        }
        break;
    case WM_LBUTTONDBLCLK:
        if (handleMainWindowLButtonDoubleClick(hwnd, lparam)) {
            return 0;
        }
        break;
    case WM_LBUTTONDOWN:
        if (handleMainWindowLButtonDown(hwnd, lparam)) {
            return 0;
        }
        break;
    case WM_RBUTTONDOWN:
        if (handleMainWindowRButtonDown(hwnd, lparam)) {
            return 0;
        }
        break;
    case WM_LBUTTONUP:
        if (handleMainWindowLButtonUp(hwnd)) {
            return 0;
        }
        break;
    case WM_CAPTURECHANGED:
        handleMainWindowCaptureChanged(hwnd, lparam);
        break;
    case WM_KEYDOWN:
        if (handleMainWindowKeyDown(hwnd, wparam)) {
            return 0;
        }
        break;
    case WM_PAINT:
        paintWindow(hwnd);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace ovtr::win32
