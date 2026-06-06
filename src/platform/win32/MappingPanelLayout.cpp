#include "platform/win32/MappingPanelLayout.h"
#include "platform/win32/MappingModel.h"
namespace ovtr::win32 {
namespace {
constexpr int kPanelPadding = 10;
constexpr int kProfileBoxHeight = 28;
constexpr int kProfileBoxGap = 8;
constexpr int kColorBoxHeight = 28;
constexpr int kColorBoxGap = 4;
constexpr int kNameBoxHeight = 28;
constexpr int kNameBoxGap = 4;
constexpr int kPresetControlsGap = 4;
constexpr int kPresetControlsHeight = 28;
constexpr int kActorButtonGap = 6;
constexpr int kActorButtonHeight = 28;
constexpr int kActorListGap = 8;
constexpr int kActorListHeight = 84;
constexpr int kMinimumActorListHeight = 56;
constexpr int kFilterBoxGap = 8;
constexpr int kFilterBoxHeight = 34;
constexpr int kPreferredRowHeight = 28;
constexpr int kMinimumRowHeight = 18;
constexpr int kDropdownGap = 2;
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

int visibleDropdownOptionCount(const int optionCount) noexcept
{
    return optionCount <= 0 ? 1 : optionCount;
}
} // namespace
MappingPanelControlsLayout mappingControlsLayoutForPanel(const ProfilePanelLayout& panelLayout) noexcept
{
    MappingPanelControlsLayout layout;
    if (!panelLayout.valid || panelLayout.panelRect.right <= panelLayout.panelRect.left) {
        return layout;
    }

    const RECT contentRect = insetRect(panelLayout.panelRect, kPanelPadding, kPanelPadding);
    const int fixedWithoutActorList = kProfileBoxHeight + kProfileBoxGap + kColorBoxHeight + kColorBoxGap +
        kNameBoxHeight + kNameBoxGap + kNameBoxHeight + kNameBoxGap +
        kPresetControlsGap + kPresetControlsHeight + kActorButtonGap +
        kActorButtonHeight + kActorListGap + kActorButtonGap + kActorButtonHeight + kFilterBoxGap + kFilterBoxHeight;
    int actorListHeight = kActorListHeight;
    int availableTableHeight = contentRect.bottom - contentRect.top - fixedWithoutActorList - actorListHeight;
    if (availableTableHeight < kMinimumRowHeight) {
        actorListHeight = kMinimumActorListHeight;
        availableTableHeight = contentRect.bottom - contentRect.top - fixedWithoutActorList - actorListHeight;
    }
    if (availableTableHeight < kMinimumRowHeight) {
        actorListHeight = 0;
        availableTableHeight = contentRect.bottom - contentRect.top - fixedWithoutActorList;
    }
    if (availableTableHeight < kMinimumRowHeight) {
        return layout;
    }

    layout.rowHeight = availableTableHeight < kPreferredRowHeight ? availableTableHeight : kPreferredRowHeight;
    layout.visibleRowCount = availableTableHeight / layout.rowHeight;
    if (layout.visibleRowCount < 1) {
        layout.visibleRowCount = 1;
    }
    if (layout.visibleRowCount > kMappingPanelRowCount) {
        layout.visibleRowCount = kMappingPanelRowCount;
    }
    layout.profileBoxRect = {contentRect.left, contentRect.top, contentRect.right, contentRect.top + kProfileBoxHeight};
    const int boxWidth = layout.profileBoxRect.right - layout.profileBoxRect.left;
    const int valueLeft = layout.profileBoxRect.left + (boxWidth * 38) / 100;
    layout.profileLabelRect = {layout.profileBoxRect.left + 8, layout.profileBoxRect.top, valueLeft - 8, layout.profileBoxRect.bottom};
    layout.profileValueRect = {valueLeft + 4, layout.profileBoxRect.top + 3, layout.profileBoxRect.right - 8, layout.profileBoxRect.bottom - 3};
    layout.tableRect = {contentRect.left, layout.profileBoxRect.bottom + kProfileBoxGap, contentRect.right,
                        layout.profileBoxRect.bottom + kProfileBoxGap + layout.rowHeight * layout.visibleRowCount};
    layout.colorBoxRect = {contentRect.left, layout.tableRect.bottom + kColorBoxGap, contentRect.right,
                           layout.tableRect.bottom + kColorBoxGap + kColorBoxHeight};
    layout.colorLabelRect = {layout.colorBoxRect.left + 8, layout.colorBoxRect.top, valueLeft - 8, layout.colorBoxRect.bottom};
    layout.colorPickButtonRect = {layout.colorBoxRect.right - 86, layout.colorBoxRect.top + 3, layout.colorBoxRect.right - 8, layout.colorBoxRect.bottom - 3};
    layout.colorSwatchRect = {valueLeft + 4, layout.colorBoxRect.top + 5, layout.colorPickButtonRect.left - 8, layout.colorBoxRect.bottom - 5};
    layout.actorNameBoxRect = {contentRect.left, layout.colorBoxRect.bottom + kNameBoxGap, contentRect.right,
                               layout.colorBoxRect.bottom + kNameBoxGap + kNameBoxHeight};
    layout.actorNameLabelRect = {layout.actorNameBoxRect.left + 8, layout.actorNameBoxRect.top, valueLeft - 8, layout.actorNameBoxRect.bottom};
    layout.actorNameValueRect = {valueLeft + 4, layout.actorNameBoxRect.top + 3, layout.actorNameBoxRect.right - 8, layout.actorNameBoxRect.bottom - 3};
    layout.nameBoxRect = {contentRect.left, layout.actorNameBoxRect.bottom + kNameBoxGap, contentRect.right,
                          layout.actorNameBoxRect.bottom + kNameBoxGap + kNameBoxHeight};
    layout.nameLabelRect = {layout.nameBoxRect.left + 8, layout.nameBoxRect.top, valueLeft - 8, layout.nameBoxRect.bottom};
    layout.nameValueRect = {valueLeft + 4, layout.nameBoxRect.top + 3, layout.nameBoxRect.right - 8, layout.nameBoxRect.bottom - 3};
    const int presetTop = layout.nameBoxRect.bottom + kPresetControlsGap;
    const int saveRight = contentRect.left + 86;
    layout.presetSaveButtonRect = {contentRect.left, presetTop, saveRight, presetTop + kPresetControlsHeight};
    layout.presetValueRect = {saveRight + 8, presetTop, contentRect.right, presetTop + kPresetControlsHeight};
    const int addTop = layout.presetSaveButtonRect.bottom + kActorButtonGap;
    layout.addActorButtonRect = {contentRect.left, addTop, contentRect.right, addTop + kActorButtonHeight};
    const int actorListTop = layout.addActorButtonRect.bottom + kActorListGap;
    layout.actorListRect = {contentRect.left, actorListTop, contentRect.right, actorListTop + actorListHeight};
    const int calibrateTop = layout.actorListRect.bottom + kActorButtonGap;
    layout.calibrateButtonRect = {contentRect.left, calibrateTop, contentRect.right, calibrateTop + kActorButtonHeight};
    const int filterTop = layout.calibrateButtonRect.bottom + kFilterBoxGap;
    layout.filterBoxRect = {contentRect.left, filterTop, contentRect.right, filterTop + kFilterBoxHeight};
    const int filterWidth = layout.filterBoxRect.right - layout.filterBoxRect.left;
    const int filterValueLeft = layout.filterBoxRect.left + (filterWidth * 48) / 100;
    const int filterMidY = layout.filterBoxRect.top + kFilterBoxHeight / 2;
    layout.filterArmLabelRect = {layout.filterBoxRect.left + 8, layout.filterBoxRect.top, filterValueLeft - 8, filterMidY};
    layout.filterArmValueRect = {filterValueLeft + 4, layout.filterBoxRect.top + 3, layout.filterBoxRect.right - 8, filterMidY - 3};
    layout.filterLegLabelRect = {layout.filterBoxRect.left + 8, filterMidY, filterValueLeft - 8, layout.filterBoxRect.bottom};
    layout.filterLegValueRect = {filterValueLeft + 4, filterMidY + 3, layout.filterBoxRect.right - 8, layout.filterBoxRect.bottom - 3};
    layout.valid = true;
    return layout;
}

int maxMappingScrollOffset(const int visibleRowCount) noexcept
{
    if (visibleRowCount <= 0 || visibleRowCount >= kMappingPanelRowCount) {
        return 0;
    }
    return kMappingPanelRowCount - visibleRowCount;
}

int clampMappingScrollOffset(const int scrollOffset, const int visibleRowCount) noexcept
{
    const int maxOffset = maxMappingScrollOffset(visibleRowCount);
    if (scrollOffset < 0) {
        return 0;
    }
    return scrollOffset > maxOffset ? maxOffset : scrollOffset;
}

std::vector<MappingPanelRowLayout> mappingRowLayoutsForPanel(const ProfilePanelLayout& panelLayout, const int scrollOffset)
{
    std::vector<MappingPanelRowLayout> rows;
    const MappingPanelControlsLayout controls = mappingControlsLayoutForPanel(panelLayout);
    if (!controls.valid) {
        return rows;
    }
    rows.reserve(static_cast<std::size_t>(controls.visibleRowCount));
    const int tableWidth = controls.tableRect.right - controls.tableRect.left;
    const int valueLeft = controls.tableRect.left + (tableWidth * 38) / 100;
    const bool showScrollbar = maxMappingScrollOffset(controls.visibleRowCount) > 0;
    const int valueRight = controls.tableRect.right - (showScrollbar ? 18 : 8);
    const int firstRow = clampMappingScrollOffset(scrollOffset, controls.visibleRowCount);
    for (int visibleRow = 0; visibleRow < controls.visibleRowCount; ++visibleRow) {
        const int panelRow = firstRow + visibleRow;
        const int rowTop = controls.tableRect.top + visibleRow * controls.rowHeight;
        MappingPanelRowLayout row;
        row.rowRect = {controls.tableRect.left, rowTop, controls.tableRect.right, rowTop + controls.rowHeight};
        row.labelRect = {row.rowRect.left + 8, row.rowRect.top, valueLeft - 8, row.rowRect.bottom};
        row.valueRect = {valueLeft + 4, row.rowRect.top + 3, valueRight, row.rowRect.bottom - 3};
        row.slotIndex = panelRow;
        row.valid = true;
        rows.push_back(row);
    }
    return rows;
}

MappingPanelRowLayout mappingRowLayoutAtPoint(const ProfilePanelLayout& panelLayout, const POINT point, const int scrollOffset)
{
    for (const MappingPanelRowLayout& row : mappingRowLayoutsForPanel(panelLayout, scrollOffset)) {
        if (PtInRect(&row.valueRect, point)) {
            return row;
        }
    }
    return {};
}

MappingPanelRowLayout mappingRowLayoutForSlot(const ProfilePanelLayout& panelLayout, const int slotIndex, const int scrollOffset)
{
    for (const MappingPanelRowLayout& row : mappingRowLayoutsForPanel(panelLayout, scrollOffset)) {
        if (row.slotIndex == slotIndex) {
            return row;
        }
    }
    return {};
}

RECT mappingDropdownRectForRow(
    const MappingPanelRowLayout& row,
    const ProfilePanelLayout& panelLayout,
    const int optionCount
) noexcept
{
    if (!row.valid) {
        return RECT{0, 0, 0, 0};
    }
    const int rowHeight = row.rowRect.bottom - row.rowRect.top;
    const int visibleOptions = visibleDropdownOptionCount(optionCount);
    const int popupHeight = rowHeight * visibleOptions;
    RECT rect{row.valueRect.left, row.rowRect.bottom + kDropdownGap, row.valueRect.right,
              row.rowRect.bottom + kDropdownGap + popupHeight};
    const int panelBottom = panelLayout.panelRect.bottom - kPanelPadding;
    const int panelTop = panelLayout.panelRect.top + kPanelPadding;
    if (rect.bottom > panelBottom && row.rowRect.top - kDropdownGap - popupHeight >= panelTop) {
        rect.bottom = row.rowRect.top - kDropdownGap;
        rect.top = rect.bottom - popupHeight;
    }
    return rect;
}

int mappingDropdownOptionFromPoint(
    const MappingPanelRowLayout& row,
    const ProfilePanelLayout& panelLayout,
    const int optionCount,
    const POINT point
) noexcept
{
    const RECT rect = mappingDropdownRectForRow(row, panelLayout, optionCount);
    if (!PtInRect(&rect, point)) {
        return -1;
    }
    const int rowHeight = row.rowRect.bottom - row.rowRect.top;
    const int option = (point.y - rect.top) / rowHeight;
    const int visibleOptions = visibleDropdownOptionCount(optionCount);
    return option >= 0 && option < visibleOptions ? option : -1;
}

RECT mappingProfileDropdownRectForControls(
    const MappingPanelControlsLayout& controls,
    const ProfilePanelLayout& panelLayout,
    const int optionCount
) noexcept
{
    if (!controls.valid) {
        return RECT{0, 0, 0, 0};
    }
    const int rowHeight = controls.profileBoxRect.bottom - controls.profileBoxRect.top;
    const int popupHeight = rowHeight * visibleDropdownOptionCount(optionCount);
    RECT rect{controls.profileValueRect.left, controls.profileBoxRect.bottom + kDropdownGap, controls.profileValueRect.right,
              controls.profileBoxRect.bottom + kDropdownGap + popupHeight};
    const int panelBottom = panelLayout.panelRect.bottom - kPanelPadding;
    if (rect.bottom > panelBottom) {
        rect.bottom = panelBottom;
    }
    return rect;
}

int mappingProfileDropdownOptionFromPoint(
    const MappingPanelControlsLayout& controls,
    const ProfilePanelLayout& panelLayout,
    const int optionCount,
    const POINT point
) noexcept
{
    const RECT rect = mappingProfileDropdownRectForControls(controls, panelLayout, optionCount);
    if (!PtInRect(&rect, point)) {
        return -1;
    }

    const int rowHeight = controls.profileBoxRect.bottom - controls.profileBoxRect.top;
    const int option = (point.y - rect.top) / rowHeight;
    const int visibleOptions = visibleDropdownOptionCount(optionCount);
    return option >= 0 && option < visibleOptions ? option : -1;
}

RECT mappingPresetDropdownRectForControls(
    const MappingPanelControlsLayout& controls,
    const ProfilePanelLayout& panelLayout,
    const int optionCount
) noexcept
{
    if (!controls.valid) {
        return RECT{0, 0, 0, 0};
    }
    const int rowHeight = controls.presetValueRect.bottom - controls.presetValueRect.top;
    const int popupHeight = rowHeight * visibleDropdownOptionCount(optionCount);
    RECT rect{controls.presetValueRect.left, controls.presetValueRect.bottom + kDropdownGap,
              controls.presetValueRect.right, controls.presetValueRect.bottom + kDropdownGap + popupHeight};
    const int panelBottom = panelLayout.panelRect.bottom - kPanelPadding;
    const int panelTop = panelLayout.panelRect.top + kPanelPadding;
    if (rect.bottom > panelBottom && controls.presetValueRect.top - kDropdownGap - popupHeight >= panelTop) {
        rect.bottom = controls.presetValueRect.top - kDropdownGap;
        rect.top = rect.bottom - popupHeight;
    } else if (rect.bottom > panelBottom) {
        rect.bottom = panelBottom;
        if (rect.bottom < rect.top) {
            rect.top = rect.bottom;
        }
    }
    return rect;
}

int mappingPresetDropdownOptionFromPoint(const MappingPanelControlsLayout& controls,
                                         const ProfilePanelLayout& panelLayout,
                                         const int optionCount,
                                         const POINT point) noexcept
{
    const RECT rect = mappingPresetDropdownRectForControls(controls, panelLayout, optionCount);
    if (!PtInRect(&rect, point)) {
        return -1;
    }

    const int rowHeight = controls.presetValueRect.bottom - controls.presetValueRect.top;
    const int option = (point.y - rect.top) / rowHeight;
    const int visibleOptions = visibleDropdownOptionCount(optionCount);
    return option >= 0 && option < visibleOptions ? option : -1;
}
} // namespace ovtr::win32
