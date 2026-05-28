#pragma once

#include "data/SessionTypes.h"
#include "export/RenderModelGeometry.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ovtr {

struct GltfKey {
    double timeSeconds = 0.0;
    std::array<float, 3> translation{0.0f, 0.0f, 0.0f};
    std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};
};

struct GltfDevice {
    DeviceDescriptor device;
    std::string nodeName;
    int nodeIndex = -1;
    int meshIndex = -1;
    std::vector<GltfKey> keys;
    RenderModelGeometry geometry;
};

struct GltfMeshPrimitive {
    std::string name;
    int positionAccessor = -1;
    int normalAccessor = -1;
    int indexAccessor = -1;
};

struct GltfAnimationTarget {
    int node = -1;
    int timeAccessor = -1;
    int translationAccessor = -1;
    int rotationAccessor = -1;
};

} // namespace ovtr
