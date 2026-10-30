#include "platform/win32/MappingPanelPainter.h"
#include "platform/win32/UiTheme.h"

#include "platform/win32/AppState.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/MappingActorLayout.h"
#include "platform/win32/MappingModel.h"
#include "platform/win32/MappingPanelColorPainter.h"
#include "platform/win32/MappingPanelDropdownPainter.h"
#include "platform/win32/MappingPanelLayout.h"
#include "platform/win32/MappingPanelScrollPainter.h"
#include "platform/win32/MappingSoftIkFilter.h"
#include "platform/win32/Win32GdiResources.h"

#include <cstddef>
#include <string>

namespace ovtr::win32 {
namespace {

void drawTableBox(HDC drawDc, const RECT& rect)
{
    UniqueBrush boxBrush(CreateSolidBrush(ui::field));
    FillRect(drawDc, &rect, boxBrush.get());

    UniquePen borderPen(CreatePen(PS_SOLID, 1, ui::buttonBorder));
    SelectObjectGuard penSelection(drawDc, borderPen.get());
    MoveToEx(drawDc, rect.left, rect.top, nullptr);
    LineTo(drawDc, rect.right - 1, rect.top);
    LineTo(drawDc, rect.right - 1, rect.bottom - 1);
    LineTo(drawDc, rect.left, rect.bottom - 1);
    LineTo(drawDc, rect.left, rect.top);
}

void drawDropdownGlyph(HDC drawDc, const RECT& valueRect)
{
    RECT glyphRect{valueRect.right - 18, valueRect.top, valueRect.right - 2, valueRect.bottom};
    ui::chevron(drawDc, glyphRect);
}

void drawButton(HDC drawDc, HFONT font, const RECT& rect, const wchar_t* label)
{
    UniqueBrush buttonBrush(CreateSolidBrush(ui::button));
    UniquePen buttonPen(CreatePen(PS_SOLID, 1, ui::buttonBorder));
    SelectObjectGuard brushSelection(drawDc, buttonBrush.get());
    SelectObjectGuard penSelection(drawDc, buttonPen.get());
    RoundRect(drawDc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);
    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, ui::text);
    RECT textRect = rect;
    DrawTextW(drawDc, label, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void drawProfileBox(HDC drawDc, HFONT font, const AppWindowState& state, const MappingPanelControlsLayout& controls)
{
    drawTableBox(drawDc, controls.profileBoxRect);
    UniquePen dividerPen(CreatePen(PS_SOLID, 1, ui::divider));
    {
        SelectObjectGuard penSelection(drawDc, dividerPen.get());
        MoveToEx(drawDc, controls.profileLabelRect.right + 8, controls.profileBoxRect.top, nullptr);
        LineTo(drawDc, controls.profileLabelRect.right + 8, controls.profileBoxRect.bottom);
    }

    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, ui::muted);
    RECT labelRect = controls.profileLabelRect;
    DrawTextW(drawDc, L"Profile", -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SetTextColor(drawDc, ui::text);
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
        return effectiveMappingActorName(actor);
    }
    return effectiveMappingActorName(actor) + L" calibrated";
}

void drawLabeledTextBox(
    HDC drawDc,
    HFONT font,
    const RECT& boxRect,
    const RECT& labelRect,
    const RECT& valueRect,
    const wchar_t* label,
    const std::wstring& value
)
{
    drawTableBox(drawDc, boxRect);
    UniquePen dividerPen(CreatePen(PS_SOLID, 1, ui::divider));
    SelectObjectGuard penSelection(drawDc, dividerPen.get());
    MoveToEx(drawDc, labelRect.right + 8, boxRect.top, nullptr);
    LineTo(drawDc, labelRect.right + 8, boxRect.bottom);

    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, ui::muted);
    RECT drawLabelRect = labelRect;
    DrawTextW(drawDc, label, -1, &drawLabelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SetTextColor(drawDc, ui::text);
    RECT drawValueRect = valueRect;
    DrawTextW(
        drawDc,
        value.c_str(),
        -1,
        &drawValueRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );
}

void drawActorNameBox(HDC drawDc, HFONT font, const AppWindowState& state, const MappingPanelControlsLayout& controls)
{
    drawLabeledTextBox(
        drawDc,
        font,
        controls.actorNameBoxRect,
        controls.actorNameLabelRect,
        controls.actorNameValueRect,
        L"Actor name",
        effectiveMappingActorNameText(state)
    );
}

void drawNameBox(HDC drawDc, HFONT font, const AppWindowState& state, const MappingPanelControlsLayout& controls)
{
    drawLabeledTextBox(
        drawDc,
        font,
        controls.nameBoxRect,
        controls.nameLabelRect,
        controls.nameValueRect,
        L"Preset name",
        mappingPresetNameText(state)
    );
}

void drawPresetControls(HDC drawDc, HFONT font, const MappingPanelControlsLayout& controls)
{
    drawButton(drawDc, font, controls.presetSaveButtonRect, L"Save");
    UniqueBrush valueBrush(CreateSolidBrush(ui::field));
    UniquePen valuePen(CreatePen(PS_SOLID, 1, ui::buttonBorder));
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
    SetTextColor(drawDc, ui::text);
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

    UniquePen gridPen(CreatePen(PS_SOLID, 1, ui::divider));
    SelectObjectGuard penSelection(drawDc, gridPen.get());
    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, ui::text);
    for (const MappingActorRowLayout& row :
         mappingActorRowLayouts(controls, state.mappingActors.size(), state.mappingActorScrollOffset)) {
        if (state.mappingActors[row.actorIndex].id == state.selectedMappingActorId) {
            UniqueBrush selectedBrush(CreateSolidBrush(ui::selection));
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
    drawTableBox(drawDc, controls.tableRect);
    drawMappingColorRow(drawDc, font, state, controls);
    drawActorNameBox(drawDc, font, state, controls);
    drawNameBox(drawDc, font, state, controls);
    drawPresetControls(drawDc, font, controls);
    drawActorList(drawDc, font, state, controls);
    drawMappingSoftIkFilter(drawDc, font, state, controls);

    UniquePen gridPen(CreatePen(PS_SOLID, 1, ui::divider));
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
        SetTextColor(drawDc, ui::muted);
        DrawTextW(
            drawDc,
            mappingPanelRowLabel(row.slotIndex),
            -1,
            &labelRect,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
        );
        SetTextColor(drawDc, ui::text);
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
        } else if (isMappingFingerRow(state.mappingDropdownSlot)) {
                drawMappingDeviceDropdown(
                drawDc,
                font,
                state,
                layout,
                makeFingerInputRows(state, mappingFingerSideIndexForRow(state.mappingDropdownSlot))
            );
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
