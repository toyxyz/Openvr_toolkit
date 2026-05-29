#include "platform/win32/ConfigTextInternal.h"

#include <algorithm>

namespace ovtr::win32 {

RgbColor clampRgbColor(const RgbColor color) noexcept
{
    return {
        clampColorComponent(color.r),
        clampColorComponent(color.g),
        clampColorComponent(color.b),
    };
}

ViewportSettings clampViewportSettings(ViewportSettings settings) noexcept
{
    settings.labelTextColor = clampRgbColor(settings.labelTextColor);
    settings.gridColor = clampRgbColor(settings.gridColor);
    settings.backgroundColor = clampRgbColor(settings.backgroundColor);
    settings.importedGlbColor = clampRgbColor(settings.importedGlbColor);
    settings.renderModelOutlineColor = clampRgbColor(settings.renderModelOutlineColor);
    settings.renderModelMaterialColor = clampRgbColor(settings.renderModelMaterialColor);
    settings.fingerBoxColor = clampRgbColor(settings.fingerBoxColor);
    settings.markerColor = clampRgbColor(settings.markerColor);
    settings.outlineMultiplier = std::clamp(settings.outlineMultiplier, 0.0f, 10.0f);
    settings.gridSize = std::clamp(settings.gridSize, 1.0f, 50.0f);
    settings.gridCellDensity = std::clamp(settings.gridCellDensity, 0.25f, 10.0f);
    settings.markerSize = std::clamp(settings.markerSize, 0.01f, 2.0f);
    return settings;
}

} // namespace ovtr::win32
