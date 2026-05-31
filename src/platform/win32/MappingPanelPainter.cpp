#include "platform/win32/MappingPanelPainter.h"

#include "platform/win32/AppState.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/MappingActorLayout.h"
#include "platform/win32/MappingModel.h"
#include "platform/win32/MappingPanelDropdownPainter.h"
#include "platform/win32/MappingPanelLayout.h"
#include "platform/win32/MappingSoftIkFilter.h"
#include "platform/win32/Win32GdiResources.h"

#include <cstddef>
#include <string>

namespace ovtr::win32 {
namespace {

void drawTableBox(HDC drawDc, const RECT& rect)
{
    UniqueBrush boxBrush(CreateSolidBrush(RGB(18, 22, 28)));
    FillRect(drawDc, &rect, boxBrush.get());

    UniquePen borderPen(CreatePen(PS_SOLID, 1, RGB(67, 74, 88)));
    SelectObjectGuard penSelection(drawDc, borderPen.get());
    MoveToEx(drawDc, rect.left, rect.top, nullptr);
    LineTo(drawDc, rect.right - 1, rect.top);
    LineTo(drawDc, rect.right - 1, rect.bottom - 1);
    LineTo(drawDc, rect.left, rect.bottom - 1);
    LineTo(drawDc, rect.left, rect.top);
}

void drawScrollbar(HDC drawDc, const RECT& tableRect, const int visibleRows, const int scrollOffset)
{
    const int maxOffset = maxMappingScrollOffset(visibleRows);
    if (maxOffset <= 0) {
        return;
    }

    RECT track{tableRect.right - 8, tableRect.top, tableRect.right, tableRect.bottom};
    UniqueBrush trackBrush(CreateSolidBrush(RGB(34, 38, 47)));
    FillRect(drawDc, &track, trackBrush.get());

    const int trackHeight = track.bottom - track.top;
    int thumbHeight = (trackHeight * visibleRows) / (visibleRows + maxOffset);
    if (thumbHeight < 18) {
        thumbHeight = 18;
    }
    if (thumbHeight > trackHeight) {
        thumbHeight = trackHeight;
    }

    const int thumbTop = track.top + (scrollOffset * (trackHeight - thumbHeight)) / maxOffset;
    RECT thumb{track.left, thumbTop, track.right, thumbTop + thumbHeight};
    UniqueBrush thumbBrush(CreateSolidBrush(RGB(88, 101, 123)));
    FillRect(drawDc, &thumb, thumbBrush.get());
}

void drawActorScrollbar(HDC drawDc, const MappingPanelControlsLayout& controls, const AppWindowState& state)
{
    const int visibleRows = visibleMappingActorRowCount(controls);
    const int maxOffset = maxMappingActorScrollOffset(state.mappingActors.size(), visibleRows);
    if (maxOffset <= 0) {
        return;
    }
    RECT track{controls.actorListRect.right - 8, controls.actorListRect.top, controls.actorListRect.right, controls.actorListRect.bottom};
    UniqueBrush trackBrush(CreateSolidBrush(RGB(34, 38, 47)));
    FillRect(drawDc, &track, trackBrush.get());
    const int trackHeight = track.bottom - track.top;
    int thumbHeight = (trackHeight * visibleRows) / (visibleRows + maxOffset);
    if (thumbHeight < 18) {
        thumbHeight = 18;
    }
    const int top = track.top + (state.mappingActorScrollOffset * (trackHeight - thumbHeight)) / maxOffset;
    RECT thumb{track.left, top, track.right, top + thumbHeight};
    UniqueBrush thumbBrush(CreateSolidBrush(RGB(88, 101, 123)));
    FillRect(drawDc, &thumb, thumbBrush.get());
}

void drawDropdownGlyph(HDC drawDc, const RECT& valueRect)
{
    RECT glyphRect{valueRect.right - 18, valueRect.top, valueRect.right - 2, valueRect.bottom};
    DrawTextW(drawDc, L"v", -1, &glyphRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void drawButton(HDC drawDc, HFONT font, const RECT& rect, const wchar_t* label)
{
    UniqueBrush buttonBrush(CreateSolidBrush(RGB(30, 34, 42)));
    UniquePen buttonPen(CreatePen(PS_SOLID, 1, RGB(67, 74, 88)));
    SelectObjectGuard brushSelection(drawDc, buttonBrush.get());
    SelectObjectGuard penSelection(drawDc, buttonPen.get());
    RoundRect(drawDc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);
    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, RGB(225, 231, 240));
    RECT textRect = rect;
    DrawTextW(drawDc, label, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void drawProfileBox(HDC drawDc, HFONT font, const AppWindowState& state, const MappingPanelControlsLayout& controls)
{
    drawTableBox(drawDc, controls.profileBoxRect);
    UniquePen dividerPen(CreatePen(PS_SOLID, 1, RGB(43, 48, 59)));
    {
        SelectObjectGuard penSelection(drawDc, dividerPen.get());
        MoveToEx(drawDc, controls.profileLabelRect.right + 8, controls.profileBoxRect.top, nullptr);
        LineTo(drawDc, controls.profileLabelRect.right + 8, controls.profileBoxRect.bottom);
    }

    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, RGB(168, 180, 196));
    RECT labelRect = controls.profileLabelRect;
    DrawTextW(drawDc, L"Profile", -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SetTextColor(drawDc, RGB(225, 231, 240));
    RECT valueRect = controls.profileValueRect;
    valueRect.right -= 18;
    DrawTextW(drawDc, state.profile.name.c_str(), -1, &valueRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    drawDropdownGlyph(drawDc, controls.profileValueRect);
}

std::wstring mappingPresetNameText(const AppWindowState& state)
{
    return state.mappingPresetName.empty() ? state.profile.name : state.mappingPresetName;
}

std::wstring actorDisplayName(const MappingActor& actor)
{
    if (!actor.calibrated) {
        return actor.profile.name;
    }
    if (actor.liveTrackingLost) {
        return actor.profile.name + L" tracking lost";
    }
    if (actor.liveLimited) {
        return actor.profile.name + L" limited";
    }
    return actor.profile.name + L" calibrated";
}

void drawNameBox(HDC drawDc, HFONT font, const AppWindowState& state, const MappingPanelControlsLayout& controls)
{
    drawTableBox(drawDc, controls.nameBoxRect);
    UniquePen dividerPen(CreatePen(PS_SOLID, 1, RGB(43, 48, 59)));
    SelectObjectGuard penSelection(drawDc, dividerPen.get());
    MoveToEx(drawDc, controls.nameLabelRect.right + 8, controls.nameBoxRect.top, nullptr);
    LineTo(drawDc, controls.nameLabelRect.right + 8, controls.nameBoxRect.bottom);

    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, RGB(168, 180, 196));
    RECT labelRect = controls.nameLabelRect;
    DrawTextW(drawDc, L"Name", -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SetTextColor(drawDc, RGB(225, 231, 240));
    RECT valueRect = controls.nameValueRect;
    DrawTextW(
        drawDc,
        mappingPresetNameText(state).c_str(),
        -1,
        &valueRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );
}

void drawPresetControls(HDC drawDc, HFONT font, const MappingPanelControlsLayout& controls)
{
    drawButton(drawDc, font, controls.presetSaveButtonRect, L"Save");
    UniqueBrush valueBrush(CreateSolidBrush(RGB(18, 22, 28)));
    UniquePen valuePen(CreatePen(PS_SOLID, 1, RGB(67, 74, 88)));
    SelectObjectGuard brushSelection(drawDc, valueBrush.get());
    SelectObjectGuard penSelection(drawDc, valuePen.get());
    Rectangle(
        drawDc,
        controls.presetValueRect.left,
        controls.presetValueRect.top,
        controls.presetValueRect.right,
        controls.presetValueRect.bottom
    );
    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, RGB(225, 231, 240));
    RECT textRect = controls.presetValueRect;
    textRect.left += 8;
    textRect.right -= 18;
    DrawTextW(drawDc, L"Preset", -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    drawDropdownGlyph(drawDc, controls.presetValueRect);
}

void drawActorList(HDC drawDc, HFONT font, const AppWindowState& state, const MappingPanelControlsLayout& controls)
{
    drawButton(drawDc, font, controls.addActorButtonRect, L"Add actor");
    drawTableBox(drawDc, controls.actorListRect);

    UniquePen gridPen(CreatePen(PS_SOLID, 1, RGB(43, 48, 59)));
    SelectObjectGuard penSelection(drawDc, gridPen.get());
    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, RGB(225, 231, 240));
    for (const MappingActorRowLayout& row :
         mappingActorRowLayouts(controls, state.mappingActors.size(), state.mappingActorScrollOffset)) {
        if (state.mappingActors[row.actorIndex].id == state.selectedMappingActorId) {
            UniqueBrush selectedBrush(CreateSolidBrush(RGB(48, 78, 118)));
            FillRect(drawDc, &row.rowRect, selectedBrush.get());
        }
        MoveToEx(drawDc, controls.actorListRect.left, row.rowRect.bottom, nullptr);
        LineTo(drawDc, controls.actorListRect.right, row.rowRect.bottom);
        RECT nameRect = row.nameRect;
        const MappingActor& actor = state.mappingActors[row.actorIndex];
        const std::wstring name = actorDisplayName(actor);
        DrawTextW(drawDc, name.c_str(), -1, &nameRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    drawActorScrollbar(drawDc, controls, state);
    drawButton(drawDc, font, controls.calibrateButtonRect, L"Calibrate");
}

} // namespace

void paintMappingPanelContent(
    HDC drawDc,
    HFONT font,
    const AppWindowState& state,
    const ProfilePanelLayout& layout
)
{
    const MappingPanelControlsLayout controls = mappingControlsLayoutForPanel(layout);
    if (!controls.valid) {
        return;
    }

    SetBkMode(drawDc, TRANSPARENT);
    SelectObject(drawDc, font);
    drawProfileBox(drawDc, font, state, controls);
    drawNameBox(drawDc, font, state, controls);
    drawTableBox(drawDc, controls.tableRect);
    drawPresetControls(drawDc, font, controls);
    drawActorList(drawDc, font, state, controls);
    drawMappingSoftIkFilter(drawDc, font, state, controls);

    UniquePen gridPen(CreatePen(PS_SOLID, 1, RGB(43, 48, 59)));
    {
        SelectObjectGuard penSelection(drawDc, gridPen.get());
        const int dividerX = controls.tableRect.left +
            ((controls.tableRect.right - controls.tableRect.left) * 38) / 100;
        MoveToEx(drawDc, dividerX, controls.tableRect.top, nullptr);
        LineTo(drawDc, dividerX, controls.tableRect.bottom);
        for (const MappingPanelRowLayout& row : mappingRowLayoutsForPanel(layout, state.mappingScrollOffset)) {
            MoveToEx(drawDc, controls.tableRect.left, row.rowRect.bottom, nullptr);
            LineTo(drawDc, controls.tableRect.right, row.rowRect.bottom);
        }
    }

    for (const MappingPanelRowLayout& row : mappingRowLayoutsForPanel(layout, state.mappingScrollOffset)) {
        RECT labelRect = row.labelRect;
        RECT valueRect = row.valueRect;
        SetTextColor(drawDc, RGB(168, 180, 196));
        DrawTextW(
            drawDc,
            mappingPanelRowLabel(row.slotIndex),
            -1,
            &labelRect,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
        );
        SetTextColor(drawDc, RGB(225, 231, 240));
        const std::wstring value = currentMappingText(state, row.slotIndex);
        valueRect.right -= 18;
        DrawTextW(drawDc, value.c_str(), -1, &valueRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        drawDropdownGlyph(drawDc, row.valueRect);
    }

    drawScrollbar(
        drawDc,
        controls.tableRect,
        controls.visibleRowCount,
        clampMappingScrollOffset(state.mappingScrollOffset, controls.visibleRowCount)
    );
    if (state.mappingDropdownSlot >= 0) {
        if (isMappingDeviceRow(state.mappingDropdownSlot)) {
            drawMappingDeviceDropdown(drawDc, font, state, layout, makeDeviceListRows(state));
        } else if (isMappingSoftIkRow(state.mappingDropdownSlot)) {
            drawMappingSoftIkFilterDropdown(drawDc, font, state, controls);
        }
    }
    if (state.mappingProfileDropdownOpen) {
        drawMappingProfileDropdown(drawDc, font, layout, controls);
    }
    if (state.mappingPresetDropdownOpen) {
        drawMappingPresetDropdown(drawDc, font, layout, controls);
    }
}

} // namespace ovtr::win32
