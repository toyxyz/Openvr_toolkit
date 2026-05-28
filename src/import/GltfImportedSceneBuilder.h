#pragma once

#include "import/GltfGlbReader.h"
#include "import/GltfImporter.h"
#include "import/GltfJson.h"

#include <filesystem>
#include <string>

namespace ovtr {

bool buildImportedGltfScene(
    const JsonValue& root,
    const GltfGlbPayload& payload,
    const std::filesystem::path& inputPath,
    ImportedGltfScene& scene,
    std::string& error
);

} // namespace ovtr
