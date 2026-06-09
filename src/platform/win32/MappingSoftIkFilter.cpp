#include "platform/win32/MappingSoftIkFilter.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/MappingModel.h"
#include "platform/win32/Win32GdiResources.h"

#include <cmath>
#include <cstddef>

namespace ovtr::win32 {
namespace {

constexpr int kDropdownGap = 2;

std::wstring softIkText(const float strength)
{
    return std::to_wstring(static_cast<int>(std::lround(strength * 100.0f))) + L"%";
}

const wchar_t* enabledText(const bool enabled) noexcept
{
    return enabled ? L"Enabled" : L"Disabled";
}

void drawBox(HDC drawDc, const RECT& rect)
{
    UniqueBrush brush(CreateSolidBrush(RGB(18, 22, 28)));
    FillRect(drawDc, &rect, brush.get());
    UniquePen pen(CreatePen(PS_SOLID, 1, RGB(67, 74, 88)));
    SelectObjectGuard penSelection(drawDc, pen.get());
    MoveToEx(drawDc, rect.left, rect.top, nullptr);
    LineTo(drawDc, rect.right - 1, rect.top);
    LineTo(drawDc, rect.right - 1, rect.bottom - 1);
    LineTo(drawDc, rect.left, rect.bottom - 1);
    LineTo(drawDc, rect.left, rect.top);
}

void drawGlyph(HDC drawDc, const RECT& valueRect)
{
    RECT glyph{valueRect.right - 18, valueRect.top, valueRect.right - 2, valueRect.bottom};
    DrawTextW(drawDc, L"v", -1, &glyph, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

RECT valueRectForSlot(const MappingPanelControlsLayout& controls, const int slot) noexcept
{
    if (slot == kMappingArmSoftIkSlot) {
        return controls.filterArmValueRect;
    }
    if (slot == kMappingLegSoftIkSlot) {
        return controls.filterLegValueRect;
    }
    if (slot == kMappingPinHandSlot) {
        return controls.filterPinHandValueRect;
    }
    if (slot == kMappingPinFootSlot) {
        return controls.filterPinFootValueRect;
    }
    return RECT{0, 0, 0, 0};
}

RECT dropdownRectForValue(const RECT& valueRect) noexcept
{
    const int rowHeight = valueRect.bottom - valueRect.top + 6;
    const int popupHeight = rowHeight * static_cast<int>(kMappingSoftIkStrengthOptions.size());
    return RECT{
        valueRect.left,
        valueRect.top - kDropdownGap - popupHeight,
        valueRect.right,
        valueRect.top - kDropdownGap
    };
}

void refreshFilter(HWND hwnd, const AppWindowState& state)
{
    InvalidateRect(hwnd, nullptr, FALSE);
    if (state.glWindow) {
        InvalidateRect(state.glWindow, nullptr, FALSE);
        UpdateWindow(state.glWindow);
    }
}

} // namespace

void drawMappingSoftIkFilter(HDC drawDc, HFONT font, const AppWindowState& state, const MappingPanelControlsLayout& controls)
{
    drawBox(drawDc, controls.filterBoxRect);
    UniquePen pen(CreatePen(PS_SOLID, 1, RGB(43, 48, 59)));
    SelectObjectGuard penSelection(drawDc, pen.get());
    MoveToEx(drawDc, controls.filterArmLabelRect.right + 8, controls.filterBoxRect.top, nullptr);
    LineTo(drawDc, controls.filterArmLabelRect.right + 8, controls.filterBoxRect.bottom);
    MoveToEx(drawDc, controls.filterBoxRect.left, controls.filterLegLabelRect.top, nullptr);
    LineTo(drawDc, controls.filterBoxRect.right, controls.filterLegLabelRect.top);
    MoveToEx(drawDc, controls.filterBoxRect.left, controls.filterPinHandLabelRect.top, nullptr);
    LineTo(drawDc, controls.filterBoxRect.right, controls.filterPinHandLabelRect.top);
    MoveToEx(drawDc, controls.filterBoxRect.left, controls.filterPinFootLabelRect.top, nullptr);
    LineTo(drawDc, controls.filterBoxRect.right, controls.filterPinFootLabelRect.top);

    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, RGB(168, 180, 196));
    RECT armLabel = controls.filterArmLabelRect;
    RECT legLabel = controls.filterLegLabelRect;
    RECT pinHandLabel = controls.filterPinHandLabelRect;
    RECT pinFootLabel = controls.filterPinFootLabelRect;
    DrawTextW(drawDc, L"Arm Soft IK", -1, &armLabel, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    DrawTextW(drawDc, L"Leg Soft IK", -1, &legLabel, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    DrawTextW(drawDc, L"Pin Hand", -1, &pinHandLabel, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    DrawTextW(drawDc, L"Pin Foot", -1, &pinFootLabel, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SetTextColor(drawDc, RGB(225, 231, 240));
    RECT armValue = controls.filterArmValueRect;
    RECT legValue = controls.filterLegValueRect;
    RECT pinHandValue = controls.filterPinHandValueRect;
    RECT pinFootValue = controls.filterPinFootValueRect;
    armValue.right -= 18;
    legValue.right -= 18;
    DrawTextW(drawDc, softIkText(state.mappingArmSoftIkStrength).c_str(), -1, &armValue, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawTextW(drawDc, softIkText(state.mappingLegSoftIkStrength).c_str(), -1, &legValue, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawTextW(drawDc, enabledText(state.mappingPinHandTargets), -1, &pinHandValue, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawTextW(drawDc, enabledText(state.mappingPinFootTargets), -1, &pinFootValue, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    drawGlyph(drawDc, controls.filterArmValueRect);
    drawGlyph(drawDc, controls.filterLegValueRect);
}

void drawMappingSoftIkFilterDropdown(
    HDC drawDc,
    HFONT font,
    const AppWindowState& state,
    const MappingPanelControlsLayout& controls
) {
    if (!isMappingSoftIkRow(state.mappingDropdownSlot)) {
        return;
    }
    const RECT valueRect = valueRectForSlot(controls, state.mappingDropdownSlot);
    const RECT popup = dropdownRectForValue(valueRect);
    drawBox(drawDc, popup);

    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, RGB(225, 231, 240));
    const int rowHeight = valueRect.bottom - valueRect.top + 6;
    for (int option = 0; option < static_cast<int>(kMappingSoftIkStrengthOptions.size()); ++option) {
        RECT optionRect{popup.left + 6, popup.top + option * rowHeight, popup.right - 6, popup.top + (option + 1) * rowHeight};
        const std::wstring text = softIkText(kMappingSoftIkStrengthOptions[static_cast<std::size_t>(option)]);
        DrawTextW(drawDc, text.c_str(), -1, &optionRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
}

bool selectMappingSoftIkFilterDropdownOption(
    HWND hwnd,
    AppWindowState& state,
    const MappingPanelControlsLayout& controls,
    const POINT point
) {
    if (!isMappingSoftIkRow(state.mappingDropdownSlot)) {
        return false;
    }
    const RECT valueRect = valueRectForSlot(controls, state.mappingDropdownSlot);
    const RECT popup = dropdownRectForValue(valueRect);
    if (!PtInRect(&popup, point)) {
        return false;
    }
    const int rowHeight = valueRect.bottom - valueRect.top + 6;
    const int option = (point.y - popup.top) / rowHeight;
    if (option < 0 || option >= static_cast<int>(kMappingSoftIkStrengthOptions.size())) {
        return false;
    }
    const float strength = kMappingSoftIkStrengthOptions[static_cast<std::size_t>(option)];
    if (state.mappingDropdownSlot == kMappingArmSoftIkSlot) {
        state.mappingArmSoftIkStrength = strength;
    } else {
        state.mappingLegSoftIkStrength = strength;
    }
    syncMappingSoftIkStrengthsToActors(state);
    state.mappingDropdownSlot = -1;
    appendDebugLog(state, L"Mapping Soft IK filter selected");
    refreshFilter(hwnd, state);
    return true;
}

bool toggleMappingSoftIkFilterDropdown(HWND hwnd, AppWindowState& state, const MappingPanelControlsLayout& controls, const POINT point)
{
    int slot = -1;
    if (PtInRect(&controls.filterArmValueRect, point)) {
        slot = kMappingArmSoftIkSlot;
    } else if (PtInRect(&controls.filterLegValueRect, point)) {
        slot = kMappingLegSoftIkSlot;
    } else if (PtInRect(&controls.filterPinHandValueRect, point)) {
        state.mappingPinHandTargets = !state.mappingPinHandTargets;
        syncMappingSoftIkStrengthsToActors(state);
        state.mappingDropdownSlot = -1;
        appendDebugLog(state, L"Mapping Pin Hand toggled");
        refreshFilter(hwnd, state);
        return true;
    } else if (PtInRect(&controls.filterPinFootValueRect, point)) {
        state.mappingPinFootTargets = !state.mappingPinFootTargets;
        syncMappingSoftIkStrengthsToActors(state);
        state.mappingDropdownSlot = -1;
        appendDebugLog(state, L"Mapping Pin Foot toggled");
        refreshFilter(hwnd, state);
        return true;
    } else {
        return false;
    }
    state.mappingDropdownSlot = state.mappingDropdownSlot == slot ? -1 : slot;
    state.mappingProfileDropdownOpen = false;
    state.mappingPresetDropdownOpen = false;
    refreshFilter(hwnd, state);
    return true;
}

} // namespace ovtr::win32
