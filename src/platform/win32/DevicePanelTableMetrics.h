#pragma once

#include "platform/win32/Layout.h"

namespace ovtr::win32 {

inline constexpr int kDeviceListItemHeight = 26;

struct DeviceListTableMetrics {
    int itemTextRight = 0;
    int nameDividerX = 0;
    int modelDividerX = 0;
    int nameColumnLeft = 0;
    int nameColumnRight = 0;
    int modelColumnLeft = 0;
    int modelColumnRight = 0;
    int serialColumnLeft = 0;
};

inline DeviceListTableMetrics deviceListTableMetricsForLayout(
    const DeviceListLayout& layout,
    const int totalDeviceRows
) noexcept
{
    DeviceListTableMetrics metrics;
    metrics.itemTextRight = deviceListItemTextRight(layout, totalDeviceRows);
    const int listTextWidth = metrics.itemTextRight - layout.headerRect.left;
    metrics.nameDividerX = layout.headerRect.left + (listTextWidth * 24) / 100;
    metrics.modelDividerX = layout.headerRect.left + (listTextWidth * 64) / 100;
    metrics.nameColumnLeft = layout.headerRect.left;
    metrics.nameColumnRight = metrics.nameDividerX - 10;
    metrics.modelColumnLeft = metrics.nameDividerX + 12;
    metrics.modelColumnRight = metrics.modelDividerX - 10;
    metrics.serialColumnLeft = metrics.modelDividerX + 12;
    return metrics;
}

} // namespace ovtr::win32
