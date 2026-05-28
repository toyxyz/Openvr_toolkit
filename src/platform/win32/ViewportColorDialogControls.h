#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/ViewportSettingsModel.h"

#include <array>

namespace ovtr::win32 {

inline constexpr UINT_PTR kViewportColorEditBaseControlId = 4000;
inline constexpr UINT_PTR kViewportColorPickBaseControlId = 4100;
inline constexpr UINT_PTR kViewportColorSwatchBaseControlId = 4300;
inline constexpr UINT_PTR kViewportOutlineEditControlId = 4200;
inline constexpr UINT_PTR kViewportColorResetControlId = 4201;

struct ViewportColorEditControls {
    HWND red = nullptr;
    HWND green = nullptr;
    HWND blue = nullptr;
    HWND pick = nullptr;
    HWND swatch = nullptr;
};

struct ViewportColorDialogControls {
    std::array<ViewportColorEditControls, kViewportColorSlotCount> colors{};
    HWND outlineEdit = nullptr;
};

void createViewportColorDialogControls(HWND hwnd, ViewportColorDialogControls& controls);

} // namespace ovtr::win32
