#include "platform/win32/MappingEditPanelPainter.h"

#include "platform/win32/AppState.h"
#include "platform/win32/MappingEditPanelLayout.h"
#include "platform/win32/MappingEditPresetPainter.h"
#include "platform/win32/MappingModel.h"
#include "platform/win32/Win32GdiResources.h"

#include <cstdio>
#include <cmath>
#include <string>

namespace ovtr::win32 {
namespace {

void drawBox(HDC dc, const RECT& rect)
{
    UniqueBrush brush(CreateSolidBrush(RGB(18, 22, 28)));
    FillRect(dc, &rect, brush.get());
    UniquePen pen(CreatePen(PS_SOLID, 1, RGB(67, 74, 88)));
    SelectObjectGuard penSelection(dc, pen.get());
    SelectObjectGuard brushSelection(dc, GetStockObject(NULL_BRUSH));
    Rectangle(dc, rect.left, rect.top, rect.right, rect.bottom);
}

const MappingActor* selectedActor(const AppWindowState& state) noexcept
{
    for (const MappingActor& actor : state.mappingActors) {
        if (actor.id == state.selectedMappingActorId) {
            return &actor;
        }
    }
    return nullptr;
}

void drawActorName(HDC dc, HFONT font, const MappingEditPanelLayout& layout, const MappingActor* actor)
{
    SelectObjectGuard fontSelection(dc, font);
    SetTextColor(dc, RGB(225, 231, 240));
    RECT actorName = layout.actorNameRect;
    DrawTextW(
        dc,
        actor ? actor->profile.name.c_str() : L"No actor",
        -1,
        &actorName,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );
}

std::wstring meterText(const float value)
{
    wchar_t text[32]{};
    std::swprintf(text, 32, L"%.3f m", static_cast<double>(value));
    return text;
}

std::wstring scalarText(const float value)
{
    wchar_t text[32]{};
    std::swprintf(text, 32, L"%.3f", static_cast<double>(value));
    return text;
}

bool sameStep(const float lhs, const float rhs) noexcept
{
    return std::fabs(lhs - rhs) < 0.00001f;
}

std::wstring stepText(const float value)
{
    if (sameStep(value, 0.001f)) {
        return L"0.001m";
    }
    if (sameStep(value, 0.01f)) {
        return L"0.01m";
    }
    return L"0.1m";
}

void drawButton(HDC dc, HFONT font, const RECT& rect, const wchar_t* text)
{
    UniqueBrush brush(CreateSolidBrush(RGB(30, 34, 42)));
    UniquePen pen(CreatePen(PS_SOLID, 1, RGB(67, 74, 88)));
    SelectObjectGuard brushSelection(dc, brush.get());
    SelectObjectGuard penSelection(dc, pen.get());
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 5, 5);
    SelectObjectGuard fontSelection(dc, font);
    SetTextColor(dc, RGB(225, 231, 240));
    RECT textRect = rect;
    DrawTextW(dc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void drawStepSelector(
    HDC dc,
    HFONT font,
    const AppWindowState& state,
    const ProfilePanelLayout& panel,
    const MappingEditPanelLayout& layout
)
{
    RECT label{layout.actorNameRect.left + 8, layout.stepValueRect.top, layout.stepValueRect.left - 6, layout.stepValueRect.bottom};
    SelectObjectGuard fontSelection(dc, font);
    SetTextColor(dc, RGB(168, 180, 196));
    DrawTextW(dc, L"Step", -1, &label, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    UniqueBrush brush(CreateSolidBrush(RGB(30, 34, 42)));
    UniquePen pen(CreatePen(PS_SOLID, 1, RGB(67, 74, 88)));
    SelectObjectGuard brushSelection(dc, brush.get());
    SelectObjectGuard penSelection(dc, pen.get());
    RoundRect(dc, layout.stepValueRect.left, layout.stepValueRect.top, layout.stepValueRect.right, layout.stepValueRect.bottom, 5, 5);
    RECT valueRect = layout.stepValueRect;
    valueRect.left += 8;
    valueRect.right -= 24;
    SetTextColor(dc, RGB(225, 231, 240));
    const std::wstring text = stepText(state.mappingEditOffsetStepMeters);
    DrawTextW(dc, text.c_str(), -1, &valueRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT arrowRect{layout.stepValueRect.right - 22, layout.stepValueRect.top, layout.stepValueRect.right - 6, layout.stepValueRect.bottom};
    DrawTextW(dc, L"v", -1, &arrowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    if (!state.mappingEditStepDropdownOpen) {
        return;
    }
    for (const MappingEditStepOptionLayout& option : mappingEditStepOptionsForPanel(panel)) {
        const bool selected = sameStep(option.stepMeters, state.mappingEditOffsetStepMeters);
        UniqueBrush optionBrush(CreateSolidBrush(selected ? RGB(48, 78, 118) : RGB(30, 34, 42)));
        FillRect(dc, &option.rowRect, optionBrush.get());
        SelectObjectGuard optionBorderBrush(dc, GetStockObject(NULL_BRUSH));
        Rectangle(dc, option.rowRect.left, option.rowRect.top, option.rowRect.right, option.rowRect.bottom);
        RECT textRect{option.rowRect.left + 8, option.rowRect.top, option.rowRect.right - 8, option.rowRect.bottom};
        const std::wstring optionText = stepText(option.stepMeters);
        SetTextColor(dc, RGB(225, 231, 240));
        DrawTextW(dc, optionText.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
}

void drawRows(HDC dc, HFONT font, const AppWindowState& state, const ProfilePanelLayout& panel)
{
    const MappingEditPanelLayout layout = mappingEditPanelLayoutForPanel(panel);
    const MappingActor* actor = selectedActor(state);
    drawActorName(dc, font, layout, actor);
    drawBox(dc, layout.listRect);
    const int scrollOffset = clampMappingEditOffsetScrollOffset(
        state.mappingEditOffsetScrollOffset,
        layout.visibleRowCount
    );
    SelectObjectGuard fontSelection(dc, font);
    UniquePen gridPen(CreatePen(PS_SOLID, 1, RGB(43, 48, 59)));
    SelectObjectGuard penSelection(dc, gridPen.get());
    for (const MappingEditPanelRowLayout& row : mappingEditRowsForPanel(panel, scrollOffset)) {
        if (row.slotIndex == state.selectedMappingOffsetSlot) {
            UniqueBrush selectedBrush(CreateSolidBrush(RGB(48, 78, 118)));
            FillRect(dc, &row.rowRect, selectedBrush.get());
        }
        MoveToEx(dc, layout.listRect.left, row.rowRect.bottom, nullptr);
        LineTo(dc, layout.listRect.right, row.rowRect.bottom);
        RECT label = row.labelRect;
        SetTextColor(dc, RGB(225, 231, 240));
        DrawTextW(dc, mappingPanelRowLabel(row.slotIndex), -1, &label, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    if (maxMappingEditOffsetScrollOffset(layout.visibleRowCount) > 0) {
        UniqueBrush trackBrush(CreateSolidBrush(RGB(23, 27, 34)));
        FillRect(dc, &layout.listScrollbarRect, trackBrush.get());
        const RECT thumb = mappingEditOffsetScrollbarThumbRect(layout, scrollOffset);
        UniqueBrush thumbBrush(CreateSolidBrush(RGB(88, 101, 123)));
        FillRect(dc, &thumb, thumbBrush.get());
    }
}

void drawAxisValues(HDC dc, HFONT font, const MappingActor& actor, const AppWindowState& state, const ProfilePanelLayout& panel)
{
    const MappingEditPanelLayout layout = mappingEditPanelLayoutForPanel(panel);
    drawBox(dc, layout.valuesRect);
    SelectObjectGuard fontSelection(dc, font);
    SetTextColor(dc, RGB(168, 180, 196));
    RECT title{layout.valuesRect.left + 8, layout.valuesRect.top + 4, layout.valuesRect.right - 8, layout.valuesRect.top + 26};
    DrawTextW(dc, L"Offset position", -1, &title, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT rotationTitle{layout.valuesRect.left + 8, layout.valuesRect.top + 92, layout.valuesRect.right - 8, layout.valuesRect.top + 112};
    DrawTextW(dc, L"Offset rotation", -1, &rotationTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    if (state.selectedMappingOffsetSlot < 0 || state.selectedMappingOffsetSlot >= kMappingSlotCount) {
        return;
    }

    const MappingTransform& offset =
        actor.calibration.trackerToTarget[static_cast<std::size_t>(state.selectedMappingOffsetSlot)];
    const float positionValues[3]{offset.position.x, offset.position.y, offset.position.z};
    const wchar_t* positionLabels[3]{L"X", L"Y", L"Z"};
    const wchar_t* rotationLabels[4]{L"X", L"Y", L"Z", L"W"};
    const std::vector<MappingEditAxisButton> buttons = mappingEditAxisButtonsForPanel(panel);
    for (int axis = 0; axis < 3; ++axis) {
        const RECT row{layout.valuesRect.left + 8, layout.valuesRect.top + 24 + axis * 20, layout.valuesRect.right - 8, layout.valuesRect.top + 44 + axis * 20};
        RECT label{row.left, row.top, row.left + 20, row.bottom};
        RECT valueRect{row.left + 54, row.top, row.right - 30, row.bottom};
        SetTextColor(dc, RGB(168, 180, 196));
        DrawTextW(dc, positionLabels[axis], -1, &label, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SetTextColor(dc, RGB(225, 231, 240));
        const std::wstring text = meterText(positionValues[axis]);
        DrawTextW(dc, text.c_str(), -1, &valueRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        drawButton(dc, font, buttons[axis * 2].rect, L"-");
        drawButton(dc, font, buttons[axis * 2 + 1].rect, L"+");
    }
    for (int axis = 0; axis < 4; ++axis) {
        const RECT row{layout.valuesRect.left + 8, layout.valuesRect.top + 112 + axis * 20, layout.valuesRect.right - 8, layout.valuesRect.top + 132 + axis * 20};
        RECT label{row.left, row.top, row.left + 20, row.bottom};
        RECT valueRect{row.left + 54, row.top, row.right - 30, row.bottom};
        SetTextColor(dc, RGB(168, 180, 196));
        DrawTextW(dc, rotationLabels[axis], -1, &label, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SetTextColor(dc, RGB(225, 231, 240));
        const std::wstring text = scalarText(offset.rotation[static_cast<std::size_t>(axis)]);
        DrawTextW(dc, text.c_str(), -1, &valueRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        const int buttonIndex = 6 + axis * 2;
        drawButton(dc, font, buttons[buttonIndex].rect, L"-");
        drawButton(dc, font, buttons[buttonIndex + 1].rect, L"+");
    }
}

} // namespace

void paintMappingEditPanelContent(HDC dc, HFONT font, const AppWindowState& state, const ProfilePanelLayout& panel)
{
    const MappingEditPanelLayout layout = mappingEditPanelLayoutForPanel(panel);
    if (!layout.valid) {
        return;
    }
    SetBkMode(dc, TRANSPARENT);
    const MappingActor* actor = selectedActor(state);
    if (!actor || !actor->calibrated) {
        drawActorName(dc, font, layout, actor);
        drawBox(dc, layout.listRect);
        SelectObjectGuard fontSelection(dc, font);
        SetTextColor(dc, RGB(225, 231, 240));
        RECT text{layout.listRect.left + 8, layout.listRect.top + 8, layout.listRect.right - 8, layout.listRect.bottom - 8};
        DrawTextW(dc, L"Select a calibrated actor.", -1, &text, DT_LEFT | DT_TOP | DT_WORDBREAK);
        drawStepSelector(dc, font, state, panel, layout);
        drawMappingEditPresetBox(dc, font, state, layout);
        drawMappingEditPresetDropdown(dc, font, state, panel);
        return;
    }
    drawRows(dc, font, state, panel);
    drawAxisValues(dc, font, *actor, state, panel);
    drawMappingEditPresetBox(dc, font, state, layout);
    drawStepSelector(dc, font, state, panel, layout);
    drawMappingEditPresetDropdown(dc, font, state, panel);
}

} // namespace ovtr::win32
