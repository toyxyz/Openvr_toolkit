#include "platform/win32/MappingCalibrationTargets.h"

#include "platform/win32/MappingTransformMath.h"

#include <array>

namespace ovtr::win32 {
namespace {

constexpr float kLimbPoleTargetOffsetMeters = 0.75f;

MappingTransform targetAt(const Vec3 position) noexcept
{
    return MappingTransform{position, {0.0f, 0.0f, 0.0f, 1.0f}};
}

Vec3 shifted(const Vec3 position, const float zOffset) noexcept
{
    return Vec3{position.x, position.y, position.z + zOffset};
}

} // namespace

std::array<MappingTransform, kMappingSlotCount> mappingCalibrationRestTargets(
    const BodyProfile& profile
) {
    const ProfileSkeletonJoints joints = buildProfileSkeletonJoints(profile);
    return {{
        targetAt(joints[kProfileJointHead].positionMeters),
        targetAt(joints[kProfileJointSpine2].positionMeters),
        targetAt(joints[kProfileJointHips].positionMeters),
        targetAt(shifted(joints[kProfileJointLeftArm].positionMeters, -kLimbPoleTargetOffsetMeters)),
        targetAt(shifted(joints[kProfileJointRightArm].positionMeters, -kLimbPoleTargetOffsetMeters)),
        targetAt(joints[kProfileJointLeftForeArm].positionMeters),
        targetAt(joints[kProfileJointRightForeArm].positionMeters),
        targetAt(shifted(joints[kProfileJointLeftLeg].positionMeters, kLimbPoleTargetOffsetMeters)),
        targetAt(shifted(joints[kProfileJointRightLeg].positionMeters, kLimbPoleTargetOffsetMeters)),
        targetAt(joints[kProfileJointLeftFoot].positionMeters),
        targetAt(joints[kProfileJointRightFoot].positionMeters),
    }};
}

} // namespace ovtr::win32
