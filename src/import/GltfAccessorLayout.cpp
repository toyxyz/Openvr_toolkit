#include "import/GltfAccessorLayout.h"

namespace ovtr {

int gltfAccessorComponentCount(const std::string& type)
{
    if (type == "SCALAR") {
        return 1;
    }
    if (type == "VEC2") {
        return 2;
    }
    if (type == "VEC3") {
        return 3;
    }
    if (type == "VEC4") {
        return 4;
    }
    return 0;
}

std::size_t gltfAccessorComponentByteSize(const int componentType)
{
    switch (componentType) {
    case kGltfComponentByte:
    case kGltfComponentUnsignedByte:
        return 1;
    case kGltfComponentShort:
    case kGltfComponentUnsignedShort:
        return 2;
    case kGltfComponentUnsignedInt:
    case kGltfComponentFloat:
        return 4;
    default:
        return 0;
    }
}

} // namespace ovtr
