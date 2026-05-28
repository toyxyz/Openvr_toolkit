#include "export/FbxCoordinateConversion.h"

#include "math/QuaternionUtils.h"

namespace ovtr {
namespace {

std::array<float, 4> multiplyQuaternions(
    const std::array<float, 4>& left,
    const std::array<float, 4>& right
)
{
    return {
        left[3] * right[0] + left[0] * right[3] + left[1] * right[2] - left[2] * right[1],
        left[3] * right[1] - left[0] * right[2] + left[1] * right[3] + left[2] * right[0],
        left[3] * right[2] + left[0] * right[1] - left[1] * right[0] + left[2] * right[3],
        left[3] * right[3] - left[0] * right[0] - left[1] * right[1] - left[2] * right[2],
    };
}

} // namespace

std::array<double, 3> convertFbxVector(
    const std::array<double, 3>& value,
    const FbxCoordinatePolicy policy
)
{
    if (policy == FbxCoordinatePolicy::Blender) {
        return {value[0], -value[2], value[1]};
    }
    return value;
}

std::array<float, 3> convertFbxVector(
    const std::array<float, 3>& value,
    const FbxCoordinatePolicy policy
)
{
    if (policy == FbxCoordinatePolicy::Blender) {
        return {value[0], -value[2], value[1]};
    }
    return value;
}

std::array<float, 4> convertFbxQuaternion(
    const std::array<float, 4>& quaternion,
    const FbxCoordinatePolicy policy
)
{
    const std::array<float, 4> source = normalizeQuaternion(quaternion);
    if (policy != FbxCoordinatePolicy::Blender) {
        return source;
    }

    constexpr float halfSqrt = 0.7071067811865476f;
    constexpr std::array<float, 4> openVrToBlender{halfSqrt, 0.0f, 0.0f, halfSqrt};
    constexpr std::array<float, 4> blenderToOpenVr{-halfSqrt, 0.0f, 0.0f, halfSqrt};
    return normalizeQuaternion(multiplyQuaternions(multiplyQuaternions(openVrToBlender, source), blenderToOpenVr));
}

void convertFbxGeometry(RenderModelGeometry& geometry, const FbxCoordinatePolicy policy)
{
    if (policy != FbxCoordinatePolicy::Blender) {
        return;
    }

    for (RenderModelVertex& vertex : geometry.vertices) {
        vertex.position = convertFbxVector(vertex.position, policy);
        vertex.normal = convertFbxVector(vertex.normal, policy);
    }
}

} // namespace ovtr
