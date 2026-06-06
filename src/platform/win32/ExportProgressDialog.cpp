#include "platform/win32/ExportProgressDialog.h"

#include "platform/win32/AppState.h"
#include "platform/win32/ExportProgressWorker.h"
#include "platform/win32/Win32GdiResources.h"
#include "platform/win32/Win32ScopedDcResources.h"
#include "platform/win32/Win32String.h"

#include <algorithm>
#include <string>

namespace ovtr::win32 {
namespace {

constexpr int kDialogWidth = 420;
constexpr int kDialogHeight = 138;

void fillRectColor(HDC dc, const RECT& rect, const COLORREF color)
{
    UniqueBrush brush(CreateSolidBrush(color));
    FillRect(dc, &rect, brush.get());
}

void drawLine(HDC dc, const RECT& rect, const std::wstring& text, const COLORREF color)
{
    SetTextColor(dc, color);
    RECT line = rect;
    DrawTextW(dc, text.c_str(), -1, &line, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void paintProgressDialog(HWND hwnd, AppWindowState& state)
{
    PaintDc paint(hwnd);
    HDC dc = paint.get();
    if (!dc) {
        return;
    }
    RECT client{};
    GetClientRect(hwnd, &client);
    SetBkMode(dc, TRANSPARENT);
    fillRectColor(dc, client, RGB(31, 35, 41));

    const ExportProgressSnapshot snapshot = exportProgressSnapshot(state);
    RECT title{24, 20, client.right - 24, 46};
    drawLine(dc, title, widen(snapshot.title), RGB(238, 242, 248));
    RECT detail{24, 50, client.right - 24, 74};
    drawLine(dc, detail, widen(snapshot.detail), RGB(178, 188, 202));

    RECT barOuter{24, 86, client.right - 24, 106};
    fillRectColor(dc, barOuter, RGB(18, 21, 25));
    const int innerWidth = (std::max)(0, static_cast<int>(barOuter.right - barOuter.left - 4));
    RECT barInner{barOuter.left + 2, barOuter.top + 2, barOuter.left + 2, barOuter.bottom - 2};
    barInner.right += static_cast<int>(innerWidth * std::clamp(snapshot.progress, 0.0f, 1.0f));
    fillRectColor(dc, barInner, RGB(80, 169, 255));

    const int percent = static_cast<int>(std::clamp(snapshot.progress, 0.0f, 1.0f) * 100.0f);
    RECT percentRect{24, 108, client.right - 24, 130};
    drawLine(dc, percentRect, std::to_wstring(percent) + L"%", RGB(178, 188, 202));

    UniquePen border(CreatePen(PS_SOLID, 1, RGB(82, 92, 106)));
    SelectObjectGuard selectedPen(dc, border.get());
    SelectObjectGuard hollowBrush(dc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(dc, 0, 0, client.right, client.bottom);
}

LRESULT CALLBACK exportProgressDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<LPCREATESTRUCTW>(lparam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (message) {
    case WM_PAINT:
        if (state) {
            paintProgressDialog(hwnd, *state);
            return 0;
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_CLOSE:
        return 0;
    case WM_NCDESTROY:
        if (state && state->exportProgressWindow == hwnd) {
            state->exportProgressWindow = nullptr;
        }
        break;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace

bool registerExportProgressDialogClass(HINSTANCE instance)
{
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = exportProgressDialogProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = kExportProgressDialogClassName;
    windowClass.hbrBackground = nullptr;
    return RegisterClassW(&windowClass) != 0;
}

void showExportProgressDialog(HWND owner, AppWindowState& state)
{
    if (state.exportProgressWindow && IsWindow(state.exportProgressWindow)) {
        updateExportProgressDialog(state);
        return;
    }
    RECT ownerRect{};
    GetWindowRect(owner, &ownerRect);
    const int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - kDialogWidth) / 2;
    const int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - kDialogHeight) / 2;
    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(owner, GWLP_HINSTANCE));
    state.exportProgressWindow = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        kExportProgressDialogClassName,
        L"Export Progress",
        WS_POPUP,
        x,
        y,
        kDialogWidth,
        kDialogHeight,
        owner,
        nullptr,
        instance,
        &state
    );
    if (!state.exportProgressWindow) {
        return;
    }
    ShowWindow(state.exportProgressWindow, SW_SHOWNOACTIVATE);
    SetWindowPos(state.exportProgressWindow, HWND_TOP, x, y, kDialogWidth, kDialogHeight, SWP_NOACTIVATE);
    UpdateWindow(state.exportProgressWindow);
}

void updateExportProgressDialog(AppWindowState& state)
{
    if (state.exportProgressWindow && IsWindow(state.exportProgressWindow)) {
        InvalidateRect(state.exportProgressWindow, nullptr, FALSE);
        UpdateWindow(state.exportProgressWindow);
    }
}

void hideExportProgressDialog(AppWindowState& state)
{
    if (state.exportProgressWindow && IsWindow(state.exportProgressWindow)) {
        DestroyWindow(state.exportProgressWindow);
    }
    state.exportProgressWindow = nullptr;
}

} // namespace ovtr::win32
