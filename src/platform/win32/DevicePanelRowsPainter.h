#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/DeviceList.h"
#include "platform/win32/DevicePanelTableMetrics.h"
#include "platform/win32/LayoutTypes.h"

#include <vector>

namespace ovtr::win32 {

struct AppDeviceState;

void paintDeviceListRows(
    HDC drawDc,
    HFONT bodyFont,
    const AppDeviceState& state,
    const DeviceListLayout& layout,
    const DeviceListTableMetrics& metrics,
    const std::vector<DeviceListRow>& deviceRows
);

} // namespace ovtr::win32
