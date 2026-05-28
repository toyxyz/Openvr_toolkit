#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/Layout.h"

namespace ovtr::win32 {

void paintDeviceListScrollbar(
    HDC drawDc,
    const DeviceListLayout& layout,
    int totalDeviceRows,
    int maxScrollOffset,
    int scrollOffset
);

} // namespace ovtr::win32
