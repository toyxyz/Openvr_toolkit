#include "platform/win32/Layout.h"

#include "platform/win32/OriginLayoutMetrics.h"

namespace ovtr::win32 {

std::vector<OriginStepperAxisLayout> originStepperAxisLayoutsForLayout(
    const OriginPanelLayout& layout,
    const bool rotation
)
{
    std::vector<OriginStepperAxisLayout> axes;
    if (!layout.valid) {
        return axes;
    }

    const RECT rowRect = originStepperRowRect(layout, rotation);
    const int rowHeight = rowRect.bottom - rowRect.top;
    const int rowWidth = rowRect.right - rowRect.left;
    if (rowHeight < kOriginStepperButtonSize || rowWidth <= kOriginStepperLabelWidth + 48) {
        return axes;
    }

    const int columnsLeft = rowRect.left + kOriginStepperLabelWidth;
    const int columnWidth = (rowRect.right - columnsLeft) / 3;
    if (columnWidth < 56) {
        return axes;
    }

    const int buttonTop = rowRect.top + (rowHeight - kOriginStepperButtonSize) / 2;
    const float step = rotation ? kOriginRotationStepDegrees : kOriginPositionStep;
    axes.reserve(3);
    for (int axis = 0; axis < 3; ++axis) {
        const int columnLeft = columnsLeft + axis * columnWidth;
        const int columnRight = axis == 2 ? rowRect.right : columnLeft + columnWidth;
        const int minusLeft = columnLeft + 14;
        const int plusRight = columnRight - 4;
        const RECT minusRect{
            minusLeft,
            buttonTop,
            minusLeft + kOriginStepperButtonSize,
            buttonTop + kOriginStepperButtonSize
        };
        const RECT plusRect{
            plusRight - kOriginStepperButtonSize,
            buttonTop,
            plusRight,
            buttonTop + kOriginStepperButtonSize
        };

        axes.push_back(OriginStepperAxisLayout{
            RECT{columnLeft, rowRect.top, minusRect.left - 2, rowRect.bottom},
            RECT{minusRect.right + 2, rowRect.top, plusRect.left - 2, rowRect.bottom},
            OriginStepperButton{minusRect, rotation, axis, -step, true},
            OriginStepperButton{plusRect, rotation, axis, step, true},
            rotation,
            axis,
            true,
        });
    }

    return axes;
}

std::vector<OriginStepperButton> originStepperButtonsForLayout(const OriginPanelLayout& layout)
{
    std::vector<OriginStepperButton> buttons;
    if (!layout.valid) {
        return buttons;
    }

    for (const bool rotation : {false, true}) {
        const std::vector<OriginStepperAxisLayout> axes =
            originStepperAxisLayoutsForLayout(layout, rotation);
        for (const OriginStepperAxisLayout& axis : axes) {
            if (axis.minusButton.valid) {
                buttons.push_back(axis.minusButton);
            }
            if (axis.plusButton.valid) {
                buttons.push_back(axis.plusButton);
            }
        }
    }
    return buttons;
}

OriginStepperButton originStepperButtonFromPoint(const OriginPanelLayout& layout, const POINT point)
{
    if (!layout.valid) {
        return {};
    }

    const std::vector<OriginStepperButton> buttons = originStepperButtonsForLayout(layout);
    for (const OriginStepperButton& button : buttons) {
        if (button.valid && PtInRect(&button.rect, point)) {
            return button;
        }
    }
    return {};
}

} // namespace ovtr::win32
