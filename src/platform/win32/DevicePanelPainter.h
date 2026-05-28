#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/LayoutTypes.h"

namespace ovtr::win32 {

struct AppDeviceState;
struct AppRuntimeState;
struct AppSessionState;
struct AppWindowState;

void paintDeviceListPanel(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    const AppRuntimeState& runtimeState,
    const AppSessionState& sessionState,
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

} // namespace ovtr::win32
