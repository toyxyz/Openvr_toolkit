#include "platform/win32/Layout.h"

#include "platform/win32/OriginLayoutMetrics.h"

namespace ovtr::win32 {

RECT originEditorRectForLayout(const OriginPanelLayout& layout) noexcept
{
    if (!layout.valid) {
        return RECT{0, 0, 0, 0};
    }

    RECT editRect = layout.valueRect;
    editRect.bottom = editRect.top + 26;
    const int maxBottom = layout.boxRect.bottom - kOriginPanelPadding;
    if (editRect.bottom > maxBottom) {
        editRect.bottom = maxBottom;
    }
    return editRect;
}

RECT originStepperRowRect(const OriginPanelLayout& layout, const bool rotation) noexcept
{
    if (!layout.valid) {
        return RECT{0, 0, 0, 0};
    }

    const int top = layout.valueRect.top + (rotation ? kOriginStepperRowHeight : 0);
    const int bottom = top + kOriginStepperRowHeight;
    return RECT{
        layout.valueRect.left,
        top,
        layout.valueRect.right,
        bottom < layout.valueRect.bottom ? bottom : layout.valueRect.bottom
    };
}

RECT originStepperRowLabelRect(const OriginPanelLayout& layout, const bool rotation) noexcept
{
    const RECT rowRect = originStepperRowRect(layout, rotation);
    if (rowRect.right <= rowRect.left || rowRect.bottom <= rowRect.top) {
        return RECT{0, 0, 0, 0};
    }

    const int right = rowRect.left + kOriginStepperLabelWidth < rowRect.right
        ? rowRect.left + kOriginStepperLabelWidth
        : rowRect.right;
    return RECT{rowRect.left, rowRect.top, right, rowRect.bottom};
}

} // namespace ovtr::win32
