#pragma once

#include "data/SessionTypes.h"
#include "export/FbxAsciiMath.h"
#include "export/RenderModelGeometry.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ovtr {

struct FbxPoseKey {
    double timeSeconds = 0.0;
    std::array<double, 3> translation{0.0, 0.0, 0.0};
    std::array<float, 4> rotationQuaternion{0.0f, 0.0f, 0.0f, 1.0f};
    std::array<double, 3> rotationDegrees{0.0, 0.0, 0.0};
    FbxMatrix3 rotationMatrix{
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
    };
};

struct FbxDeviceExport {
    DeviceDescriptor device;
    std::string nodeName;
    std::int64_t modelId = 0;
    std::int64_t geometryId = 0;
    std::int64_t translationNodeId = 0;
    std::int64_t rotationNodeId = 0;
    std::array<std::int64_t, 3> translationCurveIds{0, 0, 0};
    std::array<std::int64_t, 3> rotationCurveIds{0, 0, 0};
    std::vector<FbxPoseKey> keys;
    RenderModelGeometry geometry;
};

} // namespace ovtr
