#include "platform/win32/ProfilePanelLayout.h"

#include "platform/win32/ProfileModel.h"

namespace ovtr::win32 {
namespace {

constexpr int kPanelPadding = 10;
constexpr int kButtonGap = 8;
constexpr int kButtonHeight = 28;
constexpr int kPreferredRowHeight = 26;
constexpr int kMinimumRowHeight = 18;
constexpr int kFieldCount = 2 + kProfileMeasurementCount;

RECT insetRect(RECT rect, const int dx, const int dy) noexcept
{
    rect.left += dx;
    rect.right -= dx;
    rect.top += dy;
    rect.bottom -= dy;
    if (rect.right < rect.left) {
        rect.right = rect.left;
    }
    if (rect.bottom < rect.top) {
        rect.bottom = rect.top;
    }
    return rect;
}

ProfileEditTarget targetForRow(const int row) noexcept
{
    if (row == 0) {
        return ProfileEditTarget{ProfileEditKind::Name, -1};
    }
    if (row == 1) {
        return ProfileEditTarget{ProfileEditKind::Height, -1};
    }
    return ProfileEditTarget{ProfileEditKind::Measurement, row - 2};
}

} // namespace

ProfilePanelControlsLayout profileControlsLayoutForPanel(const ProfilePanelLayout& panelLayout) noexcept
{
    ProfilePanelControlsLayout layout;
    if (!panelLayout.valid || panelLayout.panelRect.right <= panelLayout.panelRect.left) {
        return layout;
    }

    const RECT contentRect = insetRect(panelLayout.panelRect, kPanelPadding, kPanelPadding);
    const int availableTableHeight = contentRect.bottom - contentRect.top -
        kButtonGap - kButtonHeight - kButtonGap - kButtonHeight;
    if (availableTableHeight < kMinimumRowHeight) {
        return layout;
    }

    layout.rowHeight = availableTableHeight < kPreferredRowHeight ? availableTableHeight : kPreferredRowHeight;
    layout.visibleRowCount = availableTableHeight / layout.rowHeight;
    if (layout.visibleRowCount < 1) {
        layout.visibleRowCount = 1;
    }
    if (layout.visibleRowCount > kFieldCount) {
        layout.visibleRowCount = kFieldCount;
    }

    const int tableBottom = contentRect.top + layout.rowHeight * layout.visibleRowCount;
    const int previewTop = tableBottom + kButtonGap;
    const int buttonTop = previewTop + kButtonHeight + kButtonGap;
    layout.tableRect = RECT{contentRect.left, contentRect.top, contentRect.right, tableBottom};
    layout.previewButtonRect = RECT{contentRect.left, previewTop, contentRect.right, previewTop + kButtonHeight};
    const int buttonWidth = (contentRect.right - contentRect.left - kButtonGap) / 2;
    layout.saveButtonRect = RECT{contentRect.left, buttonTop, contentRect.left + buttonWidth, buttonTop + kButtonHeight};
    layout.loadButtonRect = RECT{
        layout.saveButtonRect.right + kButtonGap,
        buttonTop,
        contentRect.right,
        buttonTop + kButtonHeight
    };
    layout.valid = true;
    return layout;
}

int maxProfileScrollOffset(const int visibleRowCount) noexcept
{
    if (visibleRowCount <= 0 || visibleRowCount >= kFieldCount) {
        return 0;
    }
    return kFieldCount - visibleRowCount;
}

int clampProfileScrollOffset(const int scrollOffset, const int visibleRowCount) noexcept
{
    const int maxOffset = maxProfileScrollOffset(visibleRowCount);
    if (scrollOffset < 0) {
        return 0;
    }
    return scrollOffset > maxOffset ? maxOffset : scrollOffset;
}

std::vector<ProfilePanelFieldLayout> profileFieldLayoutsForPanel(
    const ProfilePanelLayout& panelLayout,
    const int scrollOffset
)
{
    std::vector<ProfilePanelFieldLayout> rows;
    const ProfilePanelControlsLayout controls = profileControlsLayoutForPanel(panelLayout);
    if (!controls.valid) {
        return rows;
    }

    rows.reserve(static_cast<std::size_t>(controls.visibleRowCount));
    const int tableWidth = controls.tableRect.right - controls.tableRect.left;
    const int valueLeft = controls.tableRect.left + (tableWidth * 64) / 100;
    const bool showScrollbar = maxProfileScrollOffset(controls.visibleRowCount) > 0;
    const int valueRight = controls.tableRect.right - (showScrollbar ? 18 : 8);
    const int firstRow = clampProfileScrollOffset(scrollOffset, controls.visibleRowCount);
    for (int visibleRow = 0; visibleRow < controls.visibleRowCount; ++visibleRow) {
        const int profileRow = firstRow + visibleRow;
        const int rowTop = controls.tableRect.top + visibleRow * controls.rowHeight;
        ProfilePanelFieldLayout field;
        field.rowRect = RECT{controls.tableRect.left, rowTop, controls.tableRect.right, rowTop + controls.rowHeight};
        field.labelRect = RECT{field.rowRect.left + 8, field.rowRect.top, valueLeft - 8, field.rowRect.bottom};
        field.valueRect = RECT{valueLeft + 4, field.rowRect.top + 2, valueRight, field.rowRect.bottom - 2};
        field.target = targetForRow(profileRow);
        field.valid = true;
        rows.push_back(field);
    }
    return rows;
}

ProfilePanelFieldLayout profileFieldLayoutAtPoint(
    const ProfilePanelLayout& panelLayout,
    const POINT point,
    const int scrollOffset
)
{
    for (const ProfilePanelFieldLayout& field : profileFieldLayoutsForPanel(panelLayout, scrollOffset)) {
        if (PtInRect(&field.valueRect, point)) {
            return field;
        }
    }
    return {};
}

} // namespace ovtr::win32
