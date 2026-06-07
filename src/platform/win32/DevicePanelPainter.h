#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/LayoutTypes.h"

#include <vector>

namespace ovtr::win32 {

struct AppDeviceState;
struct AppRuntimeState;
struct AppWindowState;
struct DeviceListRow;

void paintDeviceListPanel(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    const AppRuntimeState& runtimeState,
    AppDeviceState& deviceState,
    const DeviceListLayout& layout
);
void paintDeviceListPanel(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    AppWindowState& state,
    const DeviceListLayout& layout
);
void paintDeviceListPanelRows(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    AppDeviceState& deviceState,
    const DeviceListLayout& layout,
    const std::vector<DeviceListRow>& deviceRows
);

} // namespace ovtr::win32
