#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/DeviceList.h"
#include "platform/win32/DevicePanelTableMetrics.h"
#include "platform/win32/LayoutTypes.h"

#include <cstdint>

namespace ovtr::win32 {

void paintEmptyDeviceListRow(
    HDC drawDc,
    const DeviceListLayout& layout,
    const DeviceListTableMetrics& metrics,
    int itemY
);

void paintDeviceListRow(
    HDC drawDc,
    const DeviceListLayout& layout,
    const DeviceListTableMetrics& metrics,
    const DeviceListRow& row,
    std::uint32_t selectedRuntimeIndex,
    int itemY,
    bool drawBottomGridLine
);

} // namespace ovtr::win32
