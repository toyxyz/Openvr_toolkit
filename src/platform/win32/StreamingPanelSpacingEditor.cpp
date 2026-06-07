#include "platform/win32/StreamingPanelEditor.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/DialogControlHelpers.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/Win32String.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

#include <cmath>
#include <exception>
#include <iomanip>
#include <sstream>
#include <string>

namespace ovtr::win32 {
namespace {

constexpr UINT_PTR kStreamingArmSpacingEditControlId = 2402;
constexpr UINT_PTR kStreamingLegSpacingEditControlId = 2403;

enum class SpacingKind {
    Arm,
    Leg,
};

float& spacingValue(AppWindowState& state, const SpacingKind kind) noexcept
{
    return kind == SpacingKind::Arm ? state.vmcArmSpacingDegrees : state.vmcLegSpacingDegrees;
}

HWND& spacingWindow(AppWindowState& state, const SpacingKind kind) noexcept
{
    return kind == SpacingKind::Arm ? state.vmcArmSpacingEditWindow : state.vmcLegSpacingEditWindow;
}

WNDPROC& spacingOriginalProc(AppWindowState& state, const SpacingKind kind) noexcept
{
    return kind == SpacingKind::Arm ? state.vmcArmSpacingEditOriginalProc : state.vmcLegSpacingEditOriginalProc;
}

std::wstring spacingName(const SpacingKind kind)
{
    return kind == SpacingKind::Arm ? L"arm" : L"leg";
}

std::wstring spacingText(const float degrees)
{
    std::wostringstream out;
    out << std::fixed << std::setprecision(3) << degrees;
    return out.str();
}

RECT spacingEditorRectForClient(AppWindowState& state, const int width, const int height, const SpacingKind kind)
{
    const StreamingPanelLayout layout = streamingPanelLayoutForClient(&state, width, height);
    if (!layout.valid || !layout.vmcVisible) {
        return RECT{0, 0, 0, 0};
    }
    RECT rect = kind == SpacingKind::Arm ? layout.armSpacingValueRect : layout.legSpacingValueRect;
    rect.left += 4;
    rect.right -= 4;
    return rect;
}

bool tryParseSpacing(const std::wstring& text, float& degrees)
{
    try {
        std::size_t parsed = 0;
        const float value = std::stof(text, &parsed);
        if (parsed != text.size() || !std::isfinite(value) || value < -180.0f || value > 180.0f) {
            return false;
        }
        degrees = value;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void closeSpacingEditor(HWND hwnd, AppWindowState& state, const SpacingKind kind)
{
    HWND& windowSlot = spacingWindow(state, kind);
    HWND editWindow = windowSlot;
    if (!editWindow) {
        return;
    }
    WNDPROC& originalProc = spacingOriginalProc(state, kind);
    if (originalProc) {
        SetWindowLongPtrW(editWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalProc));
    }
    windowSlot = nullptr;
    originalProc = nullptr;
    DestroyWindow(editWindow);
    InvalidateRect(hwnd, nullptr, FALSE);
}

bool applySpacingText(HWND hwnd, AppWindowState& state, const SpacingKind kind)
{
    HWND editWindow = spacingWindow(state, kind);
    if (!editWindow || !IsWindow(editWindow)) {
        return false;
    }
    float degrees = spacingValue(state, kind);
    const std::wstring text = trimWide(readWindowText(editWindow));
    if (!tryParseSpacing(text, degrees)) {
        appendDebugLog(state, L"Invalid VMC " + spacingName(kind) + L" spacing: " + text);
        SetWindowTextW(editWindow, spacingText(spacingValue(state, kind)).c_str());
        focusAndSelectAllText(editWindow);
        return false;
    }
    spacingValue(state, kind) = degrees;
    appendDebugLog(state, L"VMC " + spacingName(kind) + L" spacing set: " + spacingText(degrees));
    closeSpacingEditor(hwnd, state, kind);
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

LRESULT CALLBACK armSpacingEditProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    HWND parent = GetParent(hwnd);
    AppWindowState* state = parent ? appStateForWindow(parent) : nullptr;
    if (message == WM_KEYDOWN && state) {
        if (wparam == VK_RETURN) {
            applySpacingText(parent, *state, SpacingKind::Arm);
            return 0;
        }
        if (wparam == VK_ESCAPE) {
            closeStreamingArmSpacingEditor(parent, *state);
            return 0;
        }
    }
    if (message == WM_KILLFOCUS && state) {
        applySpacingText(parent, *state, SpacingKind::Arm);
        return 0;
    }
    return state && state->vmcArmSpacingEditOriginalProc
        ? CallWindowProcW(state->vmcArmSpacingEditOriginalProc, hwnd, message, wparam, lparam)
        : DefWindowProcW(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK legSpacingEditProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    HWND parent = GetParent(hwnd);
    AppWindowState* state = parent ? appStateForWindow(parent) : nullptr;
    if (message == WM_KEYDOWN && state) {
        if (wparam == VK_RETURN) {
            applySpacingText(parent, *state, SpacingKind::Leg);
            return 0;
        }
        if (wparam == VK_ESCAPE) {
            closeStreamingLegSpacingEditor(parent, *state);
            return 0;
        }
    }
    if (message == WM_KILLFOCUS && state) {
        applySpacingText(parent, *state, SpacingKind::Leg);
        return 0;
    }
    return state && state->vmcLegSpacingEditOriginalProc
        ? CallWindowProcW(state->vmcLegSpacingEditOriginalProc, hwnd, message, wparam, lparam)
        : DefWindowProcW(hwnd, message, wparam, lparam);
}

void showSpacingEditor(HWND hwnd, AppWindowState& state, const SpacingKind kind)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const RECT editRect = spacingEditorRectForClient(state, clientRect.right, clientRect.bottom, kind);
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        return;
    }
    HWND& editWindow = spacingWindow(state, kind);
    if (editWindow && IsWindow(editWindow)) {
        SetWindowTextW(editWindow, spacingText(spacingValue(state, kind)).c_str());
        MoveWindow(editWindow, editRect.left, editRect.top,
            editRect.right - editRect.left, editRect.bottom - editRect.top, TRUE);
        ShowWindow(editWindow, SW_SHOW);
        focusAndSelectAllText(editWindow);
        return;
    }

    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
    editWindow = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        spacingText(spacingValue(state, kind)).c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        editRect.left,
        editRect.top,
        editRect.right - editRect.left,
        editRect.bottom - editRect.top,
        hwnd,
        reinterpret_cast<HMENU>(kind == SpacingKind::Arm ? kStreamingArmSpacingEditControlId : kStreamingLegSpacingEditControlId),
        instance,
        nullptr
    );
    if (!editWindow) {
        appendDebugLog(state, L"Failed to open VMC spacing editor");
        return;
    }

    applyControlFont(editWindow, reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)));
    spacingOriginalProc(state, kind) = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(editWindow, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(kind == SpacingKind::Arm ? armSpacingEditProc : legSpacingEditProc))
    );
    focusAndSelectAllText(editWindow);
    InvalidateRect(hwnd, &editRect, FALSE);
}

void updateSpacingEditorLayout(HWND hwnd, AppWindowState& state, const SpacingKind kind)
{
    HWND editWindow = spacingWindow(state, kind);
    if (!editWindow || !IsWindow(editWindow)) {
        spacingWindow(state, kind) = nullptr;
        spacingOriginalProc(state, kind) = nullptr;
        return;
    }
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const RECT editRect = spacingEditorRectForClient(state, clientRect.right, clientRect.bottom, kind);
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        ShowWindow(editWindow, SW_HIDE);
        return;
    }
    MoveWindow(editWindow, editRect.left, editRect.top,
        editRect.right - editRect.left, editRect.bottom - editRect.top, TRUE);
    ShowWindow(editWindow, SW_SHOW);
}

} // namespace

void closeStreamingArmSpacingEditor(HWND hwnd, AppWindowState& state)
{
    closeSpacingEditor(hwnd, state, SpacingKind::Arm);
}

void closeStreamingLegSpacingEditor(HWND hwnd, AppWindowState& state)
{
    closeSpacingEditor(hwnd, state, SpacingKind::Leg);
}

void closeStreamingSpacingEditors(HWND hwnd, AppWindowState& state)
{
    closeStreamingArmSpacingEditor(hwnd, state);
    closeStreamingLegSpacingEditor(hwnd, state);
}

void showStreamingArmSpacingEditor(HWND hwnd, AppWindowState& state)
{
    showSpacingEditor(hwnd, state, SpacingKind::Arm);
}

void showStreamingLegSpacingEditor(HWND hwnd, AppWindowState& state)
{
    showSpacingEditor(hwnd, state, SpacingKind::Leg);
}

void updateStreamingArmSpacingEditorLayout(HWND hwnd, AppWindowState& state)
{
    updateSpacingEditorLayout(hwnd, state, SpacingKind::Arm);
}

void updateStreamingLegSpacingEditorLayout(HWND hwnd, AppWindowState& state)
{
    updateSpacingEditorLayout(hwnd, state, SpacingKind::Leg);
}

} // namespace ovtr::win32
