#include "platform/win32/SkeletonBvhMath.h"

#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"
#include "platform/win32/MappingTransformMath.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace ovtr::win32 {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kMetersToCentimeters = 100.0f;

struct Basis {
    Vec3 x{};
    Vec3 y{};
    Vec3 z{};
};

int firstChildIndex(const ProfileSkeletonJoints& joints, const int parent) noexcept
{
    for (std::size_t index = 0; index < joints.size(); ++index) {
        if (joints[index].parentIndex == parent) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

Vec3 fallbackSideFor(const Vec3 y) noexcept
{
    return std::fabs(y.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{0.0f, 0.0f, 1.0f};
}

Vec3 cross(const Vec3 a, const Vec3 b) noexcept
{
    return Vec3{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

Basis jointBasis(const ProfileSkeletonJoints& joints, const int index) noexcept
{
    const int child = firstChildIndex(joints, index);
    const int parent = joints[static_cast<std::size_t>(index)].parentIndex;
    Vec3 y = child >= 0
        ? subMappingVec3(joints[static_cast<std::size_t>(child)].positionMeters, joints[static_cast<std::size_t>(index)].positionMeters)
        : (parent >= 0
            ? subMappingVec3(joints[static_cast<std::size_t>(index)].positionMeters, joints[static_cast<std::size_t>(parent)].positionMeters)
            : Vec3{0.0f, 1.0f, 0.0f});
    y = normalizeMappingVec3Or(y, Vec3{0.0f, 1.0f, 0.0f});
    Vec3 x = joints[static_cast<std::size_t>(index)].sideHint;
    x = subMappingVec3(x, scaleMappingVec3(y, dotMappingVec3(x, y)));
    x = normalizeMappingVec3Or(x, normalizeMappingVec3Or(cross(fallbackSideFor(y), y), Vec3{1.0f, 0.0f, 0.0f}));
    const Vec3 z = normalizeMappingVec3Or(cross(x, y), Vec3{0.0f, 0.0f, 1.0f});
    return Basis{x, y, z};
}

std::array<float, 4> quaternionFromBasisDelta(const Basis& rest, const Basis& pose) noexcept
{
    const float m00 = pose.x.x * rest.x.x + pose.y.x * rest.y.x + pose.z.x * rest.z.x;
    const float m01 = pose.x.x * rest.x.y + pose.y.x * rest.y.y + pose.z.x * rest.z.y;
    const float m02 = pose.x.x * rest.x.z + pose.y.x * rest.y.z + pose.z.x * rest.z.z;
    const float m10 = pose.x.y * rest.x.x + pose.y.y * rest.y.x + pose.z.y * rest.z.x;
    const float m11 = pose.x.y * rest.x.y + pose.y.y * rest.y.y + pose.z.y * rest.z.y;
    const float m12 = pose.x.y * rest.x.z + pose.y.y * rest.y.z + pose.z.y * rest.z.z;
    const float m20 = pose.x.z * rest.x.x + pose.y.z * rest.y.x + pose.z.z * rest.z.x;
    const float m21 = pose.x.z * rest.x.y + pose.y.z * rest.y.y + pose.z.z * rest.z.y;
    const float m22 = pose.x.z * rest.x.z + pose.y.z * rest.y.z + pose.z.z * rest.z.z;
    const float trace = m00 + m11 + m22;
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        return ovtr::normalizeQuaternion({(m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s, 0.25f * s});
    }
    if (m00 > m11 && m00 > m22) {
        const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
        return ovtr::normalizeQuaternion({0.25f * s, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s});
    }
    if (m11 > m22) {
        const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
        return ovtr::normalizeQuaternion({(m01 + m10) / s, 0.25f * s, (m12 + m21) / s, (m02 - m20) / s});
    }
    const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
    return ovtr::normalizeQuaternion({(m02 + m20) / s, (m12 + m21) / s, 0.25f * s, (m10 - m01) / s});
}

std::array<float, 3> eulerXyzDegreesFromQuaternion(const std::array<float, 4>& q) noexcept
{
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    const float m10 = 2.0f * (x * y + w * z);
    const float m00 = 1.0f - 2.0f * (y * y + z * z);
    const float m21 = 2.0f * (y * z + w * x);
    const float m22 = 1.0f - 2.0f * (x * x + y * y);
    const float m20 = 2.0f * (x * z - w * y);
    const float pitchY = std::asin(std::max(-1.0f, std::min(1.0f, -m20)));
    return {
        std::atan2(m21, m22) * 180.0f / kPi,
        pitchY * 180.0f / kPi,
        std::atan2(m10, m00) * 180.0f / kPi
    };
}

} // namespace

SkeletonBvhRotationFrame makeSkeletonBvhRotationFrame(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& pose
) {
    SkeletonBvhRotationFrame frame;
    frame.rootPositionCm = {
        pose[kProfileJointHips].positionMeters.x * kMetersToCentimeters,
        pose[kProfileJointHips].positionMeters.y * kMetersToCentimeters,
        pose[kProfileJointHips].positionMeters.z * kMetersToCentimeters
    };
    std::array<std::array<float, 4>, kProfileSkeletonJointCount> world{};
    for (int index = 0; index < kProfileSkeletonJointCount; ++index) {
        world[static_cast<std::size_t>(index)] =
            quaternionFromBasisDelta(jointBasis(rest, index), jointBasis(pose, index));
        const int parent = rest[static_cast<std::size_t>(index)].parentIndex;
        const std::array<float, 4> local = parent >= 0
            ? ovtr::multiplyQuaternion(ovtr::conjugateQuaternion(world[static_cast<std::size_t>(parent)]), world[static_cast<std::size_t>(index)])
            : world[static_cast<std::size_t>(index)];
        frame.eulerDegrees[static_cast<std::size_t>(index)] =
            eulerXyzDegreesFromQuaternion(ovtr::normalizeQuaternion(local));
    }
    return frame;
}

} // namespace ovtr::win32
