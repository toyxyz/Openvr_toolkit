#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/DeviceList.h"
#include "platform/win32/LayoutTypes.h"

#include <vector>

namespace ovtr::win32 {

struct AppDeviceState;

void paintDeviceListTable(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    AppDeviceState& state,
    const DeviceListLayout& layout,
    const std::vector<DeviceListRow>& deviceRows
);

} // namespace ovtr::win32
