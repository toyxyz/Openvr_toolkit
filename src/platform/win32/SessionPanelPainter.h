#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/AppDebugUiState.h"
#include "platform/win32/LayoutTypes.h"
#include "platform/win32/RecordingSessionList.h"

namespace ovtr::win32 {

void paintSessionListPanel(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    AppDebugUiState& state,
    const SessionListLayout& layout,
    const std::vector<RecordingSessionListRow>& rows
);

} // namespace ovtr::win32
