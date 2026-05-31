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
    require(defaults.fingerBoxColor.g == 230, "default finger box g");
    require(defaults.markerColor.r == 255, "default marker color r");
    require(defaults.bodyColor.r == 255, "default body color r");
    require(
        defaults.skeletonDisplayType == ovtr::win32::SkeletonDisplayType::Box,
        "default skeleton display box"
    );
    require(win32ConfigNearlyEqual(defaults.gridSize, 5.0f), "default grid size");
    require(win32ConfigNearlyEqual(defaults.gridCellDensity, 2.0f), "default grid density");
    require(win32ConfigNearlyEqual(defaults.markerSize, 0.10f), "default marker size");

    ovtr::win32::ViewportSettings settings;
    settings.labelTextColor = {-1, 12, 300};
    settings.gridColor = {260, -10, 55};
    settings.backgroundColor = {1, 2, 3};
    settings.importedGlbColor = {9, 8, 7};
    settings.renderModelOutlineColor = {12, 300, -8};
    settings.renderModelMaterialColor = {300, 24, -1};
    settings.fingerBoxColor = {-2, 512, 33};
    settings.markerColor = {300, -1, 44};
    settings.bodyColor = {-20, 140, 333};
    settings.skeletonDisplayType = ovtr::win32::SkeletonDisplayType::Box;
    settings.outlineMultiplier = 12.0f;
    settings.gridSize = 100.0f;
    settings.gridCellDensity = 0.1f;
    settings.markerSize = 4.0f;
    const ovtr::win32::ViewportSettings clampedSettings = ovtr::win32::clampViewportSettings(settings);
    require(
        clampedSettings.labelTextColor.r == 0 &&
            clampedSettings.labelTextColor.g == 12 &&
            clampedSettings.labelTextColor.b == 255,
        "clamp viewport label color"
    );
    require(win32ConfigNearlyEqual(clampedSettings.gridSize, 50.0f), "clamp viewport grid size");
    require(win32ConfigNearlyEqual(clampedSettings.gridCellDensity, 0.25f), "clamp viewport grid density");
    require(win32ConfigNearlyEqual(clampedSettings.markerSize, 2.0f), "clamp viewport marker size");
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
    require(
        clampedSettings.fingerBoxColor.r == 0 &&
            clampedSettings.fingerBoxColor.g == 255 &&
            clampedSettings.fingerBoxColor.b == 33,
        "clamp finger box color"
    );
    require(
        clampedSettings.markerColor.r == 255 &&
            clampedSettings.markerColor.g == 0 &&
            clampedSettings.markerColor.b == 44,
        "clamp marker color"
    );
    require(
        clampedSettings.bodyColor.r == 0 &&
            clampedSettings.bodyColor.g == 140 &&
            clampedSettings.bodyColor.b == 255,
        "clamp body color"
    );
    require(
        clampedSettings.skeletonDisplayType == ovtr::win32::SkeletonDisplayType::Box,
        "clamp preserves skeleton display type"
    );

    std::istringstream viewportInput(
        "label_r=-5\n"
        "label_g=44\n"
        "glb_b=300\n"
        "render_model_outline_g=123\n"
        "render_model_material_b=77\n"
        "finger_g=88\n"
        "marker_b=99\n"
        "body_g=144\n"
        "skeleton_type=box\n"
        "outline_multiplier=12\n"
        "grid_size=12\n"
        "grid_cell_density=4\n"
        "marker_size=0.25\n"
    );
    const ovtr::win32::ViewportSettings parsedViewport =
        ovtr::win32::parseViewportSettingsConfig(viewportInput, ovtr::win32::ViewportSettings{});
    require(parsedViewport.labelTextColor.r == 0, "parse viewport clamps label r");
    require(parsedViewport.labelTextColor.g == 44, "parse viewport label g");
    require(parsedViewport.importedGlbColor.b == 255, "parse viewport legacy glb color");
    require(parsedViewport.renderModelOutlineColor.g == 123, "parse render model outline g");
    require(parsedViewport.renderModelMaterialColor.b == 77, "parse render model material b");
    require(parsedViewport.fingerBoxColor.g == 88, "parse finger box g");
    require(parsedViewport.markerColor.b == 99, "parse marker b");
    require(parsedViewport.bodyColor.g == 144, "parse body color");
    require(
        parsedViewport.skeletonDisplayType == ovtr::win32::SkeletonDisplayType::Box,
        "parse skeleton display type"
    );
    require(win32ConfigNearlyEqual(parsedViewport.outlineMultiplier, 10.0f), "parse viewport clamps outline");
    require(win32ConfigNearlyEqual(parsedViewport.gridSize, 12.0f), "parse viewport grid size");
    require(win32ConfigNearlyEqual(parsedViewport.gridCellDensity, 4.0f), "parse viewport grid density");
    require(win32ConfigNearlyEqual(parsedViewport.markerSize, 0.25f), "parse viewport marker size");

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
    require(
        serializedViewport.find("finger_g=88") != std::string::npos,
        "serialize finger box color"
    );
    require(
        serializedViewport.find("marker_b=99") != std::string::npos,
        "serialize marker color"
    );
    require(
        serializedViewport.find("body_g=144") != std::string::npos,
        "serialize body color"
    );
    require(
        serializedViewport.find("skeleton_type=box") != std::string::npos,
        "serialize skeleton display type"
    );
    require(
        serializedViewport.find("grid_size=12.000000") != std::string::npos,
        "serialize grid size"
    );
    require(
        serializedViewport.find("grid_cell_density=4.000000") != std::string::npos,
        "serialize grid density"
    );
    require(
        serializedViewport.find("marker_size=0.250000") != std::string::npos,
        "serialize marker size"
    );

    ovtr::win32::ViewportSettings slotSettings;
    ovtr::win32::setViewportColorSlot(slotSettings, 0, {-1, 12, 300});
    ovtr::win32::setViewportColorSlot(slotSettings, 1, {1, 2, 3});
    ovtr::win32::setViewportColorSlot(slotSettings, 2, {4, 5, 6});
    ovtr::win32::setViewportColorSlot(slotSettings, 3, {7, 8, 9});
    ovtr::win32::setViewportColorSlot(slotSettings, 4, {10, 11, 12});
    ovtr::win32::setViewportColorSlot(slotSettings, 5, {13, 14, 15});
    ovtr::win32::setViewportColorSlot(slotSettings, 6, {16, 17, 18});
    ovtr::win32::setViewportColorSlot(slotSettings, 7, {19, 20, 21});
    ovtr::win32::setViewportColorSlot(slotSettings, 8, {22, 23, 24});
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
    require(
        ovtr::win32::viewportColorSlot(slotSettings, 6).g == 17,
        "viewport color slot reads finger box color"
    );
    require(
        ovtr::win32::viewportColorSlot(slotSettings, 7).b == 21,
        "viewport color slot reads marker color"
    );
    require(
        ovtr::win32::viewportColorSlot(slotSettings, 8).g == 23,
        "viewport color slot reads body color"
    );
    slotSettings.outlineMultiplier = 3.5f;
    slotSettings.gridSize = 22.0f;
    slotSettings.gridCellDensity = 3.0f;
    slotSettings.markerSize = 0.75f;
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
        defaultColors.fingerBoxColor.g == ovtr::win32::ViewportSettings{}.fingerBoxColor.g,
        "viewport default color reset restores finger box color"
    );
    require(
        defaultColors.markerColor.r == ovtr::win32::ViewportSettings{}.markerColor.r,
        "viewport default color reset restores marker color"
    );
    require(
        defaultColors.bodyColor.b == ovtr::win32::ViewportSettings{}.bodyColor.b,
        "viewport default color reset restores body color"
    );
    require(
        win32ConfigNearlyEqual(defaultColors.outlineMultiplier, 3.5f),
        "viewport default color reset preserves outline"
    );
    require(
        win32ConfigNearlyEqual(defaultColors.gridSize, 22.0f),
        "viewport default color reset preserves grid size"
    );
    require(
        win32ConfigNearlyEqual(defaultColors.markerSize, 0.75f),
        "viewport default color reset preserves marker size"
    );

    const ovtr::win32::ViewportSettings defaultGrid =
        ovtr::win32::viewportSettingsWithDefaultGrid(slotSettings);
    require(
        win32ConfigNearlyEqual(defaultGrid.gridSize, ovtr::win32::ViewportSettings{}.gridSize),
        "viewport default grid reset restores grid size"
    );
    require(
        win32ConfigNearlyEqual(defaultGrid.gridCellDensity, ovtr::win32::ViewportSettings{}.gridCellDensity),
        "viewport default grid reset restores grid density"
    );
    const ovtr::win32::ViewportSettings defaultMarker =
        ovtr::win32::viewportSettingsWithDefaultMarker(slotSettings);
    require(
        win32ConfigNearlyEqual(defaultMarker.markerSize, ovtr::win32::ViewportSettings{}.markerSize),
        "viewport default marker reset restores marker size"
    );
}

} // namespace ovtr::test
