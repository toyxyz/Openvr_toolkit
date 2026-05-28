#include "platform/win32/ConfigTextInternal.h"

#include <iomanip>
#include <sstream>

namespace ovtr::win32 {

std::string serializeViewportSettingsConfig(ViewportSettings settings)
{
    settings = clampViewportSettings(settings);

    std::ostringstream output;
    output << "label_r=" << settings.labelTextColor.r << "\n"
           << "label_g=" << settings.labelTextColor.g << "\n"
           << "label_b=" << settings.labelTextColor.b << "\n"
           << "grid_r=" << settings.gridColor.r << "\n"
           << "grid_g=" << settings.gridColor.g << "\n"
           << "grid_b=" << settings.gridColor.b << "\n"
           << "background_r=" << settings.backgroundColor.r << "\n"
           << "background_g=" << settings.backgroundColor.g << "\n"
           << "background_b=" << settings.backgroundColor.b << "\n"
           << "imported_glb_r=" << settings.importedGlbColor.r << "\n"
           << "imported_glb_g=" << settings.importedGlbColor.g << "\n"
           << "imported_glb_b=" << settings.importedGlbColor.b << "\n"
           << "render_model_outline_r=" << settings.renderModelOutlineColor.r << "\n"
           << "render_model_outline_g=" << settings.renderModelOutlineColor.g << "\n"
           << "render_model_outline_b=" << settings.renderModelOutlineColor.b << "\n"
           << "render_model_material_r=" << settings.renderModelMaterialColor.r << "\n"
           << "render_model_material_g=" << settings.renderModelMaterialColor.g << "\n"
           << "render_model_material_b=" << settings.renderModelMaterialColor.b << "\n"
           << std::fixed << std::setprecision(6)
           << "outline_multiplier=" << settings.outlineMultiplier << "\n"
           << "grid_size=" << settings.gridSize << "\n"
           << "grid_cell_density=" << settings.gridCellDensity << "\n"
           << "marker_size=" << settings.markerSize << "\n";
    return output.str();
}

} // namespace ovtr::win32
