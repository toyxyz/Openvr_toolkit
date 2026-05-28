#pragma once

#include "export/FbxAsciiExporter.h"
#include "export/RenderModelGeometry.h"

#include <array>

namespace ovtr {

std::array<double, 3> convertFbxVector(
    const std::array<double, 3>& value,
    FbxCoordinatePolicy policy
);
std::array<float, 3> convertFbxVector(
    const std::array<float, 3>& value,
    FbxCoordinatePolicy policy
);
std::array<float, 4> convertFbxQuaternion(
    const std::array<float, 4>& quaternion,
    FbxCoordinatePolicy policy
);
void convertFbxGeometry(RenderModelGeometry& geometry, FbxCoordinatePolicy policy);

} // namespace ovtr
