#include "platform/win32/Layout.h"

#include "platform/win32/DeviceListLayoutMetrics.h"

namespace ovtr::win32 {
namespace {

constexpr int kRowHeight = 34;
constexpr int kTargetOnlyBoxHeight = 116;
constexpr int kVmcBoxHeight = 214;
constexpr int kVmcBoxGap = 14;
constexpr int kPanelBottomPadding = 12;
constexpr int kDropdownRows = 2;

int desiredBoxHeight(const bool vmcVisible) noexcept
{
    if (!vmcVisible) {
        return kTargetOnlyBoxHeight;
    }
    return 12 + 28 + 8 + kRowHeight + kVmcBoxGap + kVmcBoxHeight + kPanelBottomPadding;
}

RECT rowRect(const RECT& box, const int top) noexcept
{
    return RECT{box.left + 12, top, box.right - 12, top + kRowHeight};
}

void splitRow(const RECT& row, RECT& label, RECT& value) noexcept
{
    const int labelWidth = (row.right - row.left) / 3;
    label = RECT{row.left + 8, row.top, row.left + labelWidth, row.bottom};
    value = RECT{label.right + 10, row.top + 4, row.right - 8, row.bottom - 4};
}

} // namespace

StreamingPanelLayout streamingPanelLayoutForClient(
    const bool streamingPanelVisible,
    const bool vmcVisible,
    const int leftPanelWidth,
    const int contentBottom
) noexcept {
    StreamingPanelLayout layout;
    if (!streamingPanelVisible) {
        return layout;
    }

    const int boxLeft = kDeviceListToggleRailWidth + kDeviceListContentMargin - 8;
    const int boxRight = leftPanelWidth - 24;
    const int boxTop = kDeviceListTopBarHeight + 12;
    const int boxBottomLimit = contentBottom > 12 ? contentBottom - 12 : contentBottom;
    const int wantedBoxBottom = boxTop + desiredBoxHeight(vmcVisible);
    const int boxBottom = wantedBoxBottom < boxBottomLimit
        ? wantedBoxBottom
        : boxBottomLimit;
    if (boxRight <= boxLeft + 120 || boxBottom <= boxTop + 96) {
        return layout;
    }

    layout.boxRect = RECT{boxLeft, boxTop, boxRight, boxBottom};
    layout.headerRect = RECT{boxLeft + 12, boxTop + 12, boxRight - 12, boxTop + 40};
    layout.targetBoxRect = rowRect(layout.boxRect, layout.headerRect.bottom + 8);
    splitRow(layout.targetBoxRect, layout.targetLabelRect, layout.targetValueRect);
    layout.targetDropdownRect = RECT{
        layout.targetValueRect.left,
        layout.targetValueRect.bottom,
        layout.targetValueRect.right,
        layout.targetValueRect.bottom + kRowHeight * kDropdownRows
    };

    layout.vmcVisible = false;
    if (vmcVisible) {
        const int vmcTop = layout.targetBoxRect.bottom + kVmcBoxGap;
        const int vmcBottom = vmcTop + kVmcBoxHeight;
        if (vmcBottom <= layout.boxRect.bottom - kPanelBottomPadding) {
            layout.vmcVisible = true;
        }
        if (layout.vmcVisible) {
            layout.vmcBoxRect = RECT{boxLeft + 12, vmcTop, boxRight - 12, vmcBottom};
            layout.vmcHeaderRect = RECT{
                layout.vmcBoxRect.left + 8,
                vmcTop + 8,
                layout.vmcBoxRect.right - 8,
                vmcTop + 30
            };
            layout.hostBoxRect = rowRect(layout.vmcBoxRect, layout.vmcHeaderRect.bottom + 8);
            splitRow(layout.hostBoxRect, layout.hostLabelRect, layout.hostValueRect);
            layout.portBoxRect = rowRect(layout.vmcBoxRect, layout.hostBoxRect.bottom + 8);
            splitRow(layout.portBoxRect, layout.portLabelRect, layout.portValueRect);
            layout.armSpacingBoxRect = rowRect(layout.vmcBoxRect, layout.portBoxRect.bottom + 8);
            splitRow(layout.armSpacingBoxRect, layout.armSpacingLabelRect, layout.armSpacingValueRect);
            layout.legSpacingBoxRect = rowRect(layout.vmcBoxRect, layout.armSpacingBoxRect.bottom + 8);
            splitRow(layout.legSpacingBoxRect, layout.legSpacingLabelRect, layout.legSpacingValueRect);
        }
    }
    layout.valid = true;
    return layout;
}

} // namespace ovtr::win32
