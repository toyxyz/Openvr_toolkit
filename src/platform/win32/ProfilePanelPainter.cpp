#include "platform/win32/ProfilePanelPainter.h"
#include "platform/win32/UiTheme.h"

#include "platform/win32/ProfileModel.h"
#include "platform/win32/ProfilePanelLayout.h"
#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {
namespace {

const wchar_t* labelForTarget(const ProfileEditTarget& target)
{
    if (target.kind == ProfileEditKind::Name) {
        return L"Name";
    }
    if (target.kind == ProfileEditKind::Height) {
        return L"Height";
    }
    if (target.kind == ProfileEditKind::Measurement &&
        target.measurementIndex >= 0 &&
        target.measurementIndex < kProfileMeasurementCount) {
        return profileMeasurementDefinitions()[static_cast<std::size_t>(target.measurementIndex)].label;
    }
    return L"";
}

std::wstring valueForTarget(const BodyProfile& profile, const ProfileEditTarget& target)
{
    if (target.kind == ProfileEditKind::Name) {
        return profile.name;
    }
    if (target.kind == ProfileEditKind::Height) {
        return formatProfileNumber(computedProfileHeightCm(profile));
    }
    if (target.kind == ProfileEditKind::Measurement &&
        target.measurementIndex >= 0 &&
        target.measurementIndex < kProfileMeasurementCount) {
        return formatProfileNumber(profile.measurements[static_cast<std::size_t>(target.measurementIndex)]);
    }
    return {};
}

void drawButton(HDC drawDc, HFONT font, const RECT& rect, const wchar_t* label, const bool active)
{
    UniqueBrush brush(CreateSolidBrush(active ? ui::accent : ui::button));
    UniquePen pen(CreatePen(PS_SOLID, 1, active ? ui::accent : ui::buttonBorder));
    {
        SelectObjectGuard brushSelection(drawDc, brush.get());
        SelectObjectGuard penSelection(drawDc, pen.get());
        RoundRect(drawDc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);
    }

    SelectObjectGuard fontSelection(drawDc, font);
    SetBkMode(drawDc, TRANSPARENT);
    SetTextColor(drawDc, ui::text);
    RECT textRect = rect;
    DrawTextW(drawDc, label, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

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

void drawScrollbar(HDC drawDc, const RECT& tableRect, const int visibleRows, const int scrollOffset)
{
    const int maxOffset = maxProfileScrollOffset(visibleRows);
    if (maxOffset <= 0) {
        return;
    }

    RECT track{tableRect.right - 8, tableRect.top, tableRect.right, tableRect.bottom};
    UniqueBrush trackBrush(CreateSolidBrush(ui::timeline));
    FillRect(drawDc, &track, trackBrush.get());

    const int trackHeight = track.bottom - track.top;
    int thumbHeight = (trackHeight * visibleRows) / (visibleRows + maxOffset);
    if (thumbHeight < 18) {
        thumbHeight = 18;
    }
    if (thumbHeight > trackHeight) {
        thumbHeight = trackHeight;
    }

    const int travel = trackHeight - thumbHeight;
    const int thumbTop = track.top + (scrollOffset * travel) / maxOffset;
    RECT thumb{track.left, thumbTop, track.right, thumbTop + thumbHeight};
    UniqueBrush thumbBrush(CreateSolidBrush(ui::scrollbar));
    FillRect(drawDc, &thumb, thumbBrush.get());
}

} // namespace

void paintProfilePanelContent(
    HDC drawDc,
    HFONT font,
    const AppProfileState& state,
    const ProfilePanelLayout& layout
)
{
    const ProfilePanelControlsLayout controls = profileControlsLayoutForPanel(layout);
    if (!controls.valid) {
        return;
    }

    SetBkMode(drawDc, TRANSPARENT);
    SelectObject(drawDc, font);
    drawTableBox(drawDc, controls.tableRect);
    UniqueBrush headerBrush(CreateSolidBrush(ui::header));
    RECT headerRect = controls.tableRect;
    headerRect.left += 1;
    headerRect.top += 1;
    headerRect.right -= 1;
    headerRect.bottom = headerRect.top + controls.rowHeight;
    FillRect(drawDc, &headerRect, headerBrush.get());

    UniquePen gridPen(CreatePen(PS_SOLID, 1, ui::divider));
    {
        SelectObjectGuard penSelection(drawDc, gridPen.get());
        const int dividerX = controls.tableRect.left +
            ((controls.tableRect.right - controls.tableRect.left) * 64) / 100;
        MoveToEx(drawDc, dividerX, controls.tableRect.top, nullptr);
        LineTo(drawDc, dividerX, controls.tableRect.bottom);
        for (const ProfilePanelFieldLayout& row : profileFieldLayoutsForPanel(layout, state.profileScrollOffset)) {
            MoveToEx(drawDc, controls.tableRect.left, row.rowRect.bottom, nullptr);
            LineTo(drawDc, controls.tableRect.right, row.rowRect.bottom);
        }
    }

    for (const ProfilePanelFieldLayout& row : profileFieldLayoutsForPanel(layout, state.profileScrollOffset)) {
        RECT labelRect = row.labelRect;
        RECT valueRect = row.valueRect;
        const std::wstring value = valueForTarget(state.profile, row.target);
        SetTextColor(drawDc, ui::muted);
        DrawTextW(drawDc, labelForTarget(row.target), -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SetTextColor(drawDc, ui::text);
        DrawTextW(drawDc, value.c_str(), -1, &valueRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    drawScrollbar(
        drawDc,
        controls.tableRect,
        controls.visibleRowCount,
        clampProfileScrollOffset(state.profileScrollOffset, controls.visibleRowCount)
    );
    drawButton(drawDc, font, controls.previewButtonRect, L"Preview", state.profilePreviewEnabled);
    drawButton(drawDc, font, controls.saveButtonRect, L"Save", false);
    drawButton(drawDc, font, controls.loadButtonRect, L"Load", false);
}

} // namespace ovtr::win32
