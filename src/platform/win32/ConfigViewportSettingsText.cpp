#include "platform/win32/ConfigTextInternal.h"

#include <cmath>
#include <string>

namespace ovtr::win32 {

ViewportSettings parseViewportSettingsConfig(std::istream& input, ViewportSettings settings)
{
    std::string line;
    while (std::getline(input, line)) {
        detail::ConfigAssignment assignment;
        if (!detail::parseConfigAssignmentLine(line, assignment)) {
            continue;
        }

        const std::string& key = assignment.key;
        const std::string& value = assignment.value;
        int intValue = 0;
        float floatValue = 0.0f;
        if (key == "label_r" && parseIntConfigValue(value, intValue)) {
            settings.labelTextColor.r = intValue;
        } else if (key == "label_g" && parseIntConfigValue(value, intValue)) {
            settings.labelTextColor.g = intValue;
        } else if (key == "label_b" && parseIntConfigValue(value, intValue)) {
            settings.labelTextColor.b = intValue;
        } else if (key == "grid_r" && parseIntConfigValue(value, intValue)) {
            settings.gridColor.r = intValue;
        } else if (key == "grid_g" && parseIntConfigValue(value, intValue)) {
            settings.gridColor.g = intValue;
        } else if (key == "grid_b" && parseIntConfigValue(value, intValue)) {
            settings.gridColor.b = intValue;
        } else if (key == "background_r" && parseIntConfigValue(value, intValue)) {
            settings.backgroundColor.r = intValue;
        } else if (key == "background_g" && parseIntConfigValue(value, intValue)) {
            settings.backgroundColor.g = intValue;
        } else if (key == "background_b" && parseIntConfigValue(value, intValue)) {
            settings.backgroundColor.b = intValue;
        } else if ((key == "imported_glb_r" || key == "glb_r") && parseIntConfigValue(value, intValue)) {
            settings.importedGlbColor.r = intValue;
        } else if ((key == "imported_glb_g" || key == "glb_g") && parseIntConfigValue(value, intValue)) {
            settings.importedGlbColor.g = intValue;
        } else if ((key == "imported_glb_b" || key == "glb_b") && parseIntConfigValue(value, intValue)) {
            settings.importedGlbColor.b = intValue;
        } else if (key == "render_model_outline_r" && parseIntConfigValue(value, intValue)) {
            settings.renderModelOutlineColor.r = intValue;
        } else if (key == "render_model_outline_g" && parseIntConfigValue(value, intValue)) {
            settings.renderModelOutlineColor.g = intValue;
        } else if (key == "render_model_outline_b" && parseIntConfigValue(value, intValue)) {
            settings.renderModelOutlineColor.b = intValue;
        } else if (key == "render_model_material_r" && parseIntConfigValue(value, intValue)) {
            settings.renderModelMaterialColor.r = intValue;
        } else if (key == "render_model_material_g" && parseIntConfigValue(value, intValue)) {
            settings.renderModelMaterialColor.g = intValue;
        } else if (key == "render_model_material_b" && parseIntConfigValue(value, intValue)) {
            settings.renderModelMaterialColor.b = intValue;
        } else if (key == "outline_multiplier" && parseFloatConfigValue(value, floatValue) && std::isfinite(floatValue)) {
            settings.outlineMultiplier = floatValue;
        }
    }
    return clampViewportSettings(settings);
}

} // namespace ovtr::win32
