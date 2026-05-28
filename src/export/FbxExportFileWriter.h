#pragma once

#include "export/FbxAsciiExporter.h"

#include <iosfwd>
#include <string>

namespace ovtr {

struct FbxExportScene;

bool writeFbxExportScene(
    std::ostream& out,
    FbxCoordinatePolicy coordinatePolicy,
    FbxExportScene& scene,
    std::string& error
);

} // namespace ovtr
