#pragma once

#include "platform/win32/ConfigTypes.h"

namespace ovtr::win32 {

inline constexpr int kViewportColorSlotCount = 7;

RgbColor viewportColorSlot(const ViewportSettings& settings, int index) noexcept;
void setViewportColorSlot(ViewportSettings& settings, int index, RgbColor color) noexcept;
ViewportSettings viewportSettingsWithDefaultColors(ViewportSettings current) noexcept;
ViewportSettings viewportSettingsWithDefaultGrid(ViewportSettings current) noexcept;
ViewportSettings viewportSettingsWithDefaultMarker(ViewportSettings current) noexcept;

} // namespace ovtr::win32
