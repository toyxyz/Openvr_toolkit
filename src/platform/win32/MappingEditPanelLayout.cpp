#include "platform/win32/MappingEditPanelLayout.h"

#include "platform/win32/MappingModel.h"

namespace ovtr::win32 {
namespace {

constexpr int kPanelPad = 10;
constexpr int kValueBoxHeight = 192;
constexpr int kPresetBoxHeight = 74;
constexpr int kDetailHeight = kValueBoxHeight + kPanelPad + kPresetBoxHeight;
constexpr int kActorNameHeight = 26;
constexpr int kMinListRowHeight = 18;
constexpr int kButtonSize = 24;
constexpr int kStepRowHeight = 26;
constexpr int kValueRowHeight = 20;
constexpr int kPresetControlHeight = 26;

RECT positionRowRect(const MappingEditPanelLayout& layout, const int axis) noexcept
{
    const int top = layout.valuesRect.top + 24 + axis * kValueRowHeight;
    return RECT{layout.valuesRect.left + 8, top, layout.valuesRect.right - 8, top + kValueRowHeight};
}

RECT rotationRowRect(const MappingEditPanelLayout& layout, const int axis) noexcept
{
    const int top = layout.valuesRect.top + 112 + axis * kValueRowHeight;
    return RECT{layout.valuesRect.left + 8, top, layout.valuesRect.right - 8, top + kValueRowHeight};
}

} // namespace

MappingEditPanelLayout mappingEditPanelLayoutForPanel(const ProfilePanelLayout& panel) noexcept
{
    MappingEditPanelLayout layout;
    if (!panel.valid || panel.panelRect.right - panel.panelRect.left < 180) {
        return layout;
    }

    const int left = panel.panelRect.left + kPanelPad;
    const int right = panel.panelRect.right - kPanelPad;
    const int top = panel.panelRect.top + kPanelPad;
    const int bottom = panel.panelRect.bottom - kPanelPad;
    const int listTop = top + kActorNameHeight + 4;
    const int available = bottom - listTop - kPanelPad - kStepRowHeight - 4 - kDetailHeight;
    if (available < kMinListRowHeight) {
        return layout;
    }
    const int visibleRows = available / kMinListRowHeight < kMappingSlotCount
        ? available / kMinListRowHeight
        : kMappingSlotCount;
    int rowHeight = available / visibleRows;
    if (rowHeight > 24) {
        rowHeight = 24;
    }
    layout.rowHeight = rowHeight;
    layout.visibleRowCount = visibleRows;
    layout.actorNameRect = RECT{left, top, right, top + kActorNameHeight};
    layout.listRect = RECT{left, listTop, right, listTop + rowHeight * visibleRows};
    const bool showScrollbar = maxMappingEditOffsetScrollOffset(visibleRows) > 0;
    layout.listScrollbarRect = showScrollbar
        ? RECT{layout.listRect.right - 12, layout.listRect.top + 2, layout.listRect.right - 4, layout.listRect.bottom - 2}
        : RECT{0, 0, 0, 0};
    layout.listContentRect = showScrollbar
        ? RECT{layout.listRect.left, layout.listRect.top, layout.listScrollbarRect.left - 4, layout.listRect.bottom}
        : layout.listRect;
    const int stepTop = layout.listRect.bottom + kPanelPad;
    layout.stepValueRect = RECT{left + 76, stepTop, right - 8, stepTop + kStepRowHeight};
    const int valuesTop = layout.stepValueRect.bottom + kPanelPad;
    layout.valuesRect = RECT{left, valuesTop, right, valuesTop + kValueBoxHeight};
    const int presetTop = layout.valuesRect.bottom + kPanelPad;
    layout.presetBoxRect = RECT{left, presetTop, right, presetTop + kPresetBoxHeight};
    layout.presetNameLabelRect = RECT{
        layout.presetBoxRect.left + 8,
        layout.presetBoxRect.top + 8,
        layout.presetBoxRect.left + 54,
        layout.presetBoxRect.top + 8 + kPresetControlHeight
    };
    layout.presetNameEditRect = RECT{
        layout.presetNameLabelRect.right + 6,
        layout.presetNameLabelRect.top,
        layout.presetBoxRect.right - 8,
        layout.presetNameLabelRect.bottom
    };
    const int presetControlTop = layout.presetNameEditRect.bottom + 8;
    layout.presetSaveButtonRect = RECT{
        layout.presetBoxRect.left + 8,
        presetControlTop,
        layout.presetBoxRect.left + 66,
        presetControlTop + kPresetControlHeight
    };
    layout.presetValueRect = RECT{
        layout.presetSaveButtonRect.right + 8,
        presetControlTop,
        layout.presetBoxRect.right - 8,
        presetControlTop + kPresetControlHeight
    };
    layout.valid = layout.presetBoxRect.bottom <= bottom && layout.valuesRect.bottom - layout.valuesRect.top >= kValueBoxHeight;
    return layout;
}

int maxMappingEditOffsetScrollOffset(const int visibleRowCount) noexcept
{
    return visibleRowCount < kMappingSlotCount ? kMappingSlotCount - visibleRowCount : 0;
}

int clampMappingEditOffsetScrollOffset(const int scrollOffset, const int visibleRowCount) noexcept
{
    const int maxOffset = maxMappingEditOffsetScrollOffset(visibleRowCount);
    if (scrollOffset < 0) {
        return 0;
    }
    return scrollOffset > maxOffset ? maxOffset : scrollOffset;
}

RECT mappingEditOffsetScrollbarThumbRect(const MappingEditPanelLayout& layout, const int scrollOffset) noexcept
{
    const int maxOffset = maxMappingEditOffsetScrollOffset(layout.visibleRowCount);
    if (!layout.valid || maxOffset <= 0) {
        return RECT{0, 0, 0, 0};
    }
    const int trackHeight = layout.listScrollbarRect.bottom - layout.listScrollbarRect.top;
    int thumbHeight = (trackHeight * layout.visibleRowCount) / kMappingSlotCount;
    if (thumbHeight < 18) {
        thumbHeight = 18;
    }
    if (thumbHeight > trackHeight) {
        thumbHeight = trackHeight;
    }
    const int travel = trackHeight - thumbHeight;
    const int top = layout.listScrollbarRect.top +
        (clampMappingEditOffsetScrollOffset(scrollOffset, layout.visibleRowCount) * travel) / maxOffset;
    return RECT{layout.listScrollbarRect.left, top, layout.listScrollbarRect.right, top + thumbHeight};
}

std::vector<MappingEditPanelRowLayout> mappingEditRowsForPanel(const ProfilePanelLayout& panel, const int scrollOffset)
{
    std::vector<MappingEditPanelRowLayout> rows;
    const MappingEditPanelLayout layout = mappingEditPanelLayoutForPanel(panel);
    if (!layout.valid) {
        return rows;
    }
    const int firstSlot = clampMappingEditOffsetScrollOffset(scrollOffset, layout.visibleRowCount);
    rows.reserve(static_cast<std::size_t>(layout.visibleRowCount));
    for (int rowIndex = 0; rowIndex < layout.visibleRowCount; ++rowIndex) {
        const int slot = firstSlot + rowIndex;
        const int top = layout.listContentRect.top + rowIndex * layout.rowHeight;
        const RECT rowRect{layout.listContentRect.left, top, layout.listContentRect.right, top + layout.rowHeight};
        rows.push_back(MappingEditPanelRowLayout{
            rowRect,
            RECT{rowRect.left + 8, rowRect.top, rowRect.right - 8, rowRect.bottom},
            slot,
            true
        });
    }
    return rows;
}

MappingEditPanelRowLayout mappingEditRowAtPoint(
    const ProfilePanelLayout& panel,
    const int scrollOffset,
    const POINT point
)
{
    for (const MappingEditPanelRowLayout& row : mappingEditRowsForPanel(panel, scrollOffset)) {
        if (row.valid && PtInRect(&row.rowRect, point)) {
            return row;
        }
    }
    return {};
}

std::vector<MappingEditAxisButton> mappingEditAxisButtonsForPanel(const ProfilePanelLayout& panel)
{
    std::vector<MappingEditAxisButton> buttons;
    const MappingEditPanelLayout layout = mappingEditPanelLayoutForPanel(panel);
    if (!layout.valid) {
        return buttons;
    }
    buttons.reserve(14);
    for (int axis = 0; axis < 3; ++axis) {
        const RECT row = positionRowRect(layout, axis);
        buttons.push_back(MappingEditAxisButton{
            RECT{row.left + 24, row.top + 1, row.left + 24 + kButtonSize, row.top + 1 + kButtonSize},
            axis,
            -1.0f,
            false,
            true
        });
        buttons.push_back(MappingEditAxisButton{
            RECT{row.right - kButtonSize, row.top + 1, row.right, row.top + 1 + kButtonSize},
            axis,
            1.0f,
            false,
            true
        });
    }
    for (int axis = 0; axis < 4; ++axis) {
        const RECT row = rotationRowRect(layout, axis);
        buttons.push_back(MappingEditAxisButton{
            RECT{row.left + 24, row.top + 1, row.left + 24 + kButtonSize, row.top + 1 + kButtonSize},
            axis,
            -1.0f,
            true,
            true
        });
        buttons.push_back(MappingEditAxisButton{
            RECT{row.right - kButtonSize, row.top + 1, row.right, row.top + 1 + kButtonSize},
            axis,
            1.0f,
            true,
            true
        });
    }
    return buttons;
}

MappingEditAxisButton mappingEditAxisButtonAtPoint(const ProfilePanelLayout& panel, const POINT point)
{
    for (const MappingEditAxisButton& button : mappingEditAxisButtonsForPanel(panel)) {
        if (button.valid && PtInRect(&button.rect, point)) {
            return button;
        }
    }
    return {};
}

std::vector<MappingEditStepOptionLayout> mappingEditStepOptionsForPanel(const ProfilePanelLayout& panel)
{
    std::vector<MappingEditStepOptionLayout> options;
    const MappingEditPanelLayout layout = mappingEditPanelLayoutForPanel(panel);
    if (!layout.valid) {
        return options;
    }
    options.reserve(kMappingEditOffsetStepOptionsMeters.size());
    const int height = 24;
    const int firstTop = layout.stepValueRect.bottom + 2;
    for (std::size_t index = 0; index < kMappingEditOffsetStepOptionsMeters.size(); ++index) {
        const int top = firstTop + static_cast<int>(index) * height;
        options.push_back(MappingEditStepOptionLayout{
            RECT{layout.stepValueRect.left, top, layout.stepValueRect.right, top + height},
            kMappingEditOffsetStepOptionsMeters[index],
            true
        });
    }
    return options;
}

MappingEditStepOptionLayout mappingEditStepOptionAtPoint(const ProfilePanelLayout& panel, const POINT point)
{
    for (const MappingEditStepOptionLayout& option : mappingEditStepOptionsForPanel(panel)) {
        if (option.valid && PtInRect(&option.rowRect, point)) {
            return option;
        }
    }
    return {};
}

RECT mappingEditOffsetPresetDropdownRectForPanel(const ProfilePanelLayout& panel, const int optionCount) noexcept
{
    const MappingEditPanelLayout layout = mappingEditPanelLayoutForPanel(panel);
    if (!layout.valid) {
        return RECT{0, 0, 0, 0};
    }
    const int rows = optionCount > 0 ? optionCount : 1;
    const int height = (layout.presetValueRect.bottom - layout.presetValueRect.top) * rows;
    return RECT{
        layout.presetValueRect.left,
        layout.presetValueRect.bottom + 2,
        layout.presetValueRect.right,
        layout.presetValueRect.bottom + 2 + height
    };
}

int mappingEditOffsetPresetDropdownOptionFromPoint(
    const ProfilePanelLayout& panel,
    const int optionCount,
    const POINT point
) noexcept {
    const MappingEditPanelLayout layout = mappingEditPanelLayoutForPanel(panel);
    const RECT popup = mappingEditOffsetPresetDropdownRectForPanel(panel, optionCount);
    if (!layout.valid || !PtInRect(&popup, point)) {
        return -1;
    }
    const int rowHeight = layout.presetValueRect.bottom - layout.presetValueRect.top;
    if (rowHeight <= 0) {
        return -1;
    }
    return (point.y - popup.top) / rowHeight;
}

} // namespace ovtr::win32
