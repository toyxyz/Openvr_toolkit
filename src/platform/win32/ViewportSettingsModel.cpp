#include "platform/win32/ViewportSettingsModel.h"

#include "platform/win32/ConfigStore.h"

namespace ovtr::win32 {

RgbColor viewportColorSlot(const ViewportSettings& settings, const int index) noexcept
{
    if (index == 0) {
        return settings.labelTextColor;
    }
    if (index == 1) {
        return settings.gridColor;
    }
    if (index == 2) {
        return settings.backgroundColor;
    }
    if (index == 3) {
        return settings.importedGlbColor;
    }
    if (index == 4) {
        return settings.renderModelOutlineColor;
    }
    if (index == 5) {
        return settings.renderModelMaterialColor;
    }
    if (index == 6) {
        return settings.fingerBoxColor;
    }
    if (index == 7) {
        return settings.markerColor;
    }
    return settings.bodyColor;
}

void setViewportColorSlot(ViewportSettings& settings, const int index, const RgbColor color) noexcept
{
    const RgbColor clamped = clampRgbColor(color);
    if (index == 0) {
        settings.labelTextColor = clamped;
    } else if (index == 1) {
        settings.gridColor = clamped;
    } else if (index == 2) {
        settings.backgroundColor = clamped;
    } else if (index == 3) {
        settings.importedGlbColor = clamped;
    } else if (index == 4) {
        settings.renderModelOutlineColor = clamped;
    } else if (index == 5) {
        settings.renderModelMaterialColor = clamped;
    } else if (index == 6) {
        settings.fingerBoxColor = clamped;
    } else if (index == 7) {
        settings.markerColor = clamped;
    } else {
        settings.bodyColor = clamped;
    }
}

ViewportSettings viewportSettingsWithDefaultColors(ViewportSettings current) noexcept
{
    const ViewportSettings defaults;
    current.labelTextColor = defaults.labelTextColor;
    current.gridColor = defaults.gridColor;
    current.backgroundColor = defaults.backgroundColor;
    current.importedGlbColor = defaults.importedGlbColor;
    current.renderModelOutlineColor = defaults.renderModelOutlineColor;
    current.renderModelMaterialColor = defaults.renderModelMaterialColor;
    current.fingerBoxColor = defaults.fingerBoxColor;
    current.markerColor = defaults.markerColor;
    current.bodyColor = defaults.bodyColor;
    return current;
}

ViewportSettings viewportSettingsWithDefaultGrid(ViewportSettings current) noexcept
{
    const ViewportSettings defaults;
    current.gridSize = defaults.gridSize;
    current.gridCellDensity = defaults.gridCellDensity;
    return current;
}

ViewportSettings viewportSettingsWithDefaultMarker(ViewportSettings current) noexcept
{
    const ViewportSettings defaults;
    current.markerSize = defaults.markerSize;
    return current;
}

} // namespace ovtr::win32
