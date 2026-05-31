#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/LayoutTypes.h"

namespace ovtr::win32 {

struct AppWindowState;

void paintMappingPanelContent(
    HDC drawDc,
    HFONT font,
    const AppWindowState& state,
    const ProfilePanelLayout& layout
);

} // namespace ovtr::win32
