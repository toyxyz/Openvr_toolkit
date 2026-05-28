#include "TestCases.h"
#include "TestSupport.h"
#include "Win32ConfigTestSupport.h"

#include "platform/win32/ConfigStore.h"
#include "platform/win32/ViewportSettingsModel.h"

#include <sstream>
#include <string>

namespace ovtr::test {

void testWin32ViewportSettingsConfig()
{
    const ovtr::win32::RgbColor clamped = ovtr::win32::clampRgbColor({-4, 42, 300});
    require(clamped.r == 0 && clamped.g == 42 && clamped.b == 255, "clamp rgb color");

    const ovtr::win32::ViewportSettings defaults;
    require(defaults.labelTextColor.r == 255, "default label r");
    require(defaults.gridColor.g == 36, "default grid g");
    require(defaults.backgroundColor.b == 65, "default background b");
    require(defaults.importedGlbColor.g == 104, "default imported glb g");
    require(defaults.renderModelOutlineColor.g == 133, "default render model outline g");
    require(defaults.renderModelMaterialColor.b == 255, "default render model material b");

    ovtr::win32::ViewportSettings settings;
    settings.labelTextColor = {-1, 12, 300};
    settings.gridColor = {260, -10, 55};
    settings.backgroundColor = {1, 2, 3};
    settings.importedGlbColor = {9, 8, 7};
    settings.renderModelOutlineColor = {12, 300, -8};
    settings.renderModelMaterialColor = {300, 24, -1};
    settings.outlineMultiplier = 12.0f;
    const ovtr::win32::ViewportSettings clampedSettings = ovtr::win32::clampViewportSettings(settings);
    require(
        clampedSettings.labelTextColor.r == 0 &&
            clampedSettings.labelTextColor.g == 12 &&
            clampedSettings.labelTextColor.b == 255,
        "clamp viewport label color"
    );
    require(
        clampedSettings.gridColor.r == 255 &&
            clampedSettings.gridColor.g == 0 &&
            clampedSettings.gridColor.b == 55,
        "clamp viewport grid color"
    );
    require(win32ConfigNearlyEqual(clampedSettings.outlineMultiplier, 10.0f), "clamp viewport outline");
    require(
        clampedSettings.renderModelOutlineColor.g == 255 &&
            clampedSettings.renderModelOutlineColor.b == 0,
        "clamp render model outline color"
    );
    require(
        clampedSettings.renderModelMaterialColor.r == 255 &&
            clampedSettings.renderModelMaterialColor.b == 0,
        "clamp render model material color"
    );

    std::istringstream viewportInput(
        "label_r=-5\n"
        "label_g=44\n"
        "glb_b=300\n"
        "render_model_outline_g=123\n"
        "render_model_material_b=77\n"
        "outline_multiplier=12\n"
    );
    const ovtr::win32::ViewportSettings parsedViewport =
        ovtr::win32::parseViewportSettingsConfig(viewportInput, ovtr::win32::ViewportSettings{});
    require(parsedViewport.labelTextColor.r == 0, "parse viewport clamps label r");
    require(parsedViewport.labelTextColor.g == 44, "parse viewport label g");
    require(parsedViewport.importedGlbColor.b == 255, "parse viewport legacy glb color");
    require(parsedViewport.renderModelOutlineColor.g == 123, "parse render model outline g");
    require(parsedViewport.renderModelMaterialColor.b == 77, "parse render model material b");
    require(win32ConfigNearlyEqual(parsedViewport.outlineMultiplier, 10.0f), "parse viewport clamps outline");

    const std::string serializedViewport = ovtr::win32::serializeViewportSettingsConfig(parsedViewport);
    require(
        serializedViewport.find("outline_multiplier=10.000000") != std::string::npos,
        "serialize viewport precision"
    );
    require(
        serializedViewport.find("render_model_outline_g=123") != std::string::npos,
        "serialize render model outline color"
    );
    require(
        serializedViewport.find("render_model_material_b=77") != std::string::npos,
        "serialize render model material color"
    );

    ovtr::win32::ViewportSettings slotSettings;
    ovtr::win32::setViewportColorSlot(slotSettings, 0, {-1, 12, 300});
    ovtr::win32::setViewportColorSlot(slotSettings, 1, {1, 2, 3});
    ovtr::win32::setViewportColorSlot(slotSettings, 2, {4, 5, 6});
    ovtr::win32::setViewportColorSlot(slotSettings, 3, {7, 8, 9});
    ovtr::win32::setViewportColorSlot(slotSettings, 4, {10, 11, 12});
    ovtr::win32::setViewportColorSlot(slotSettings, 5, {13, 14, 15});
    require(
        slotSettings.labelTextColor.r == 0 &&
            slotSettings.labelTextColor.g == 12 &&
            slotSettings.labelTextColor.b == 255,
        "viewport color slot clamps label color"
    );
    require(
        ovtr::win32::viewportColorSlot(slotSettings, 1).b == 3,
        "viewport color slot reads grid color"
    );
    require(
        ovtr::win32::viewportColorSlot(slotSettings, 3).g == 8,
        "viewport color slot reads imported glb color"
    );
    require(
        ovtr::win32::viewportColorSlot(slotSettings, 4).b == 12,
        "viewport color slot reads render model outline color"
    );
    require(
        ovtr::win32::viewportColorSlot(slotSettings, 5).r == 13,
        "viewport color slot reads render model material color"
    );
    slotSettings.outlineMultiplier = 3.5f;
    const ovtr::win32::ViewportSettings defaultColors =
        ovtr::win32::viewportSettingsWithDefaultColors(slotSettings);
    require(
        defaultColors.labelTextColor.r == ovtr::win32::ViewportSettings{}.labelTextColor.r,
        "viewport default color reset restores label color"
    );
    require(
        defaultColors.renderModelMaterialColor.r == ovtr::win32::ViewportSettings{}.renderModelMaterialColor.r,
        "viewport default color reset restores render model material"
    );
    require(
        win32ConfigNearlyEqual(defaultColors.outlineMultiplier, 3.5f),
        "viewport default color reset preserves outline"
    );
}

} // namespace ovtr::test
