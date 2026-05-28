#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <array>

namespace ovtr::win32 {

inline constexpr UINT_PTR kOriginDialogEditBaseControlId = 4300;
inline constexpr UINT_PTR kOriginDialogEnabledControlId = 4400;

struct OriginDialogControls {
    HWND enabledCheck = nullptr;
    std::array<HWND, 3> positionEdits{};
    std::array<HWND, 3> rotationEdits{};
};

void createOriginDialogControls(HWND hwnd, OriginDialogControls& controls);

} // namespace ovtr::win32
