#include "platform/win32/MappingVirtualTargets.h"

#include "math/PoseTransform.h"
#include "platform/win32/MappingTransformMath.h"

#include <cstddef>

namespace ovtr::win32 {
namespace {

const ovtr::PoseSample* poseForRuntimeIndex(
    const ovtr::PosePollResult& poses,
    const std::uint32_t runtimeIndex
) noexcept {
    for (const ovtr::PoseSample& pose : poses.poses) {
        if (pose.runtimeIndex == runtimeIndex) {
            return &pose;
        }
    }
    return nullptr;
}

MappingVirtualTarget targetFromPose(
    const MappingTrackerRole role,
    const ovtr::PoseSample& pose,
    const MappingTransform& trackerToTarget,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees
) {
    const ovtr::PoseSample displayPose = ovtr::applyOriginToPose(
        pose,
        originEnabled,
        originOffset,
        originRotationDegrees
    );
    MappingVirtualTarget target;
    target.role = role;
    target.trackerTransform = mappingTransformFromPose(displayPose);
    target.transform = composeMappingTransforms(target.trackerTransform, trackerToTarget);
    target.valid = true;
    return target;
}

} // namespace

MappingVirtualTargetBuildResult buildMappingVirtualTargets(
    const MappingCalibrationData& calibration,
    const ovtr::PosePollResult& poses,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees,
    std::array<MappingVirtualTarget, kMappingSlotCount>& outTargets
) {
    for (int slot = 0; slot < kMappingSlotCount; ++slot) {
        const std::size_t index = static_cast<std::size_t>(slot);
        outTargets[index] = MappingVirtualTarget{mappingRoleForSlot(slot), {}, {}, false};
        const ovtr::PoseSample* pose = poseForRuntimeIndex(poses, calibration.runtimeIndices[index]);
        if (!pose || (pose->flags & ovtr::PoseFlagPoseValid) == 0) {
            return MappingVirtualTargetBuildResult{false, slot, true};
        }
        outTargets[index] = targetFromPose(
            mappingRoleForSlot(slot),
            *pose,
            calibration.trackerToTarget[index],
            originEnabled,
            originOffset,
            originRotationDegrees
        );
    }
    return MappingVirtualTargetBuildResult{true, -1, false};
}

MappingVirtualTargetBuildResult buildMappingVirtualTargetsWithFallback(
    const MappingCalibrationData& calibration,
    const ovtr::PosePollResult& poses,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& fallbackTargets,
    std::array<MappingVirtualTarget, kMappingSlotCount>& outTargets
) {
    MappingVirtualTargetBuildResult result{true, -1, false};
    for (int slot = 0; slot < kMappingSlotCount; ++slot) {
        const std::size_t index = static_cast<std::size_t>(slot);
        const MappingTrackerRole role = mappingRoleForSlot(slot);
        const ovtr::PoseSample* pose = poseForRuntimeIndex(poses, calibration.runtimeIndices[index]);
        if (pose && (pose->flags & ovtr::PoseFlagPoseValid) != 0) {
            outTargets[index] = targetFromPose(
                role,
                *pose,
                calibration.trackerToTarget[index],
                originEnabled,
                originOffset,
                originRotationDegrees
            );
            continue;
        }
        if (!fallbackTargets[index].valid) {
            return MappingVirtualTargetBuildResult{false, slot, true};
        }
        outTargets[index] = fallbackTargets[index];
        outTargets[index].role = role;
        outTargets[index].valid = true;
        result.trackingLost = true;
        if (result.failedSlot < 0) {
            result.failedSlot = slot;
        }
    }
    return result;
}

} // namespace ovtr::win32
