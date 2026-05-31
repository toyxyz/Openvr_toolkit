#include "platform/win32/MappingTransformMath.h"

#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"

#include <array>
#include <cmath>

namespace ovtr::win32 {
namespace {

std::array<float, 3> toArray(const Vec3 value) noexcept
{
    return {value.x, value.y, value.z};
}

Vec3 toVec3(const std::array<float, 3>& value) noexcept
{
    return Vec3{value[0], value[1], value[2]};
}

} // namespace

Vec3 addMappingVec3(const Vec3 left, const Vec3 right) noexcept
{
    return Vec3{left.x + right.x, left.y + right.y, left.z + right.z};
}

Vec3 subMappingVec3(const Vec3 left, const Vec3 right) noexcept
{
    return Vec3{left.x - right.x, left.y - right.y, left.z - right.z};
}

Vec3 scaleMappingVec3(const Vec3 value, const float scale) noexcept
{
    return Vec3{value.x * scale, value.y * scale, value.z * scale};
}

float dotMappingVec3(const Vec3 left, const Vec3 right) noexcept
{
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

float lengthMappingVec3(const Vec3 value) noexcept
{
    return std::sqrt(dotMappingVec3(value, value));
}

float distanceMappingVec3(const Vec3 left, const Vec3 right) noexcept
{
    return lengthMappingVec3(subMappingVec3(left, right));
}

Vec3 normalizeMappingVec3Or(const Vec3 value, const Vec3 fallback) noexcept
{
    const float length = lengthMappingVec3(value);
    if (length <= 0.00001f) {
        return fallback;
    }
    return scaleMappingVec3(value, 1.0f / length);
}

MappingTransform identityMappingTransform() noexcept
{
    return MappingTransform{};
}

MappingTransform mappingTransformFromPose(const ovtr::PoseSample& pose) noexcept
{
    return MappingTransform{
        Vec3{pose.position[0], pose.position[1], pose.position[2]},
        ovtr::normalizeQuaternion(pose.rotation)
    };
}

MappingTransform composeMappingTransforms(
    const MappingTransform& left,
    const MappingTransform& right
) noexcept {
    const std::array<float, 3> rotated =
        ovtr::rotatePositionByQuaternion(left.rotation, toArray(right.position));
    return MappingTransform{
        addMappingVec3(left.position, toVec3(rotated)),
        ovtr::normalizeQuaternion(ovtr::multiplyQuaternion(left.rotation, right.rotation))
    };
}

MappingTransform inverseMappingTransform(const MappingTransform& transform) noexcept
{
    const std::array<float, 4> inverseRotation =
        ovtr::conjugateQuaternion(ovtr::normalizeQuaternion(transform.rotation));
    const Vec3 inversePosition = toVec3(
        ovtr::rotatePositionByQuaternion(inverseRotation, toArray(scaleMappingVec3(transform.position, -1.0f)))
    );
    return MappingTransform{inversePosition, inverseRotation};
}

Vec3 transformMappingPoint(const MappingTransform& transform, const Vec3 point) noexcept
{
    return composeMappingTransforms(transform, MappingTransform{point, {0.0f, 0.0f, 0.0f, 1.0f}}).position;
}

Vec3 rotateMappingVector(const MappingTransform& transform, const Vec3 vector) noexcept
{
    return toVec3(ovtr::rotatePositionByQuaternion(transform.rotation, toArray(vector)));
}

} // namespace ovtr::win32
