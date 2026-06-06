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

MappingVirtualTarget targetFromTransform(
    const MappingTrackerRole role,
    const MappingTransform& transform
) noexcept {
    MappingVirtualTarget target;
    target.role = role;
    target.transform = transform;
    target.valid = true;
    return target;
}

bool mappedPoseTarget(
    const MappingCalibrationData& calibration,
    const ovtr::PosePollResult& poses,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees,
    const int slot,
    MappingVirtualTarget& outTarget
) {
    const std::size_t index = static_cast<std::size_t>(slot);
    const ovtr::PoseSample* pose = poseForRuntimeIndex(poses, calibration.runtimeIndices[index]);
    if (!pose || (pose->flags & ovtr::PoseFlagPoseValid) == 0) {
        return false;
    }
    outTarget = targetFromPose(
        mappingRoleForSlot(slot),
        *pose,
        calibration.trackerToTarget[index],
        originEnabled,
        originOffset,
        originRotationDegrees
    );
    return true;
}

bool parentedPoseTarget(
    const MappingCalibrationData& calibration,
    const ovtr::PosePollResult& poses,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees,
    const int slot,
    MappingVirtualTarget& outTarget
) {
    const std::size_t index = static_cast<std::size_t>(slot);
    const int parentSlot = calibration.targetBindings[index].parentSlot;
    if (parentSlot < 0 || parentSlot >= kMappingSlotCount) {
        return false;
    }
    MappingVirtualTarget parentTarget;
    if (!mappedPoseTarget(calibration, poses, originEnabled, originOffset, originRotationDegrees, parentSlot, parentTarget)) {
        return false;
    }
    outTarget = targetFromTransform(
        mappingRoleForSlot(slot),
        composeMappingTransforms(parentTarget.trackerTransform, calibration.trackerToTarget[index])
    );
    outTarget.trackerTransform = parentTarget.trackerTransform;
    return true;
}

bool buildTargetForSlot(
    const MappingCalibrationData& calibration,
    const ovtr::PosePollResult& poses,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees,
    const int slot,
    MappingVirtualTarget& outTarget
) {
    const std::size_t index = static_cast<std::size_t>(slot);
    const MappingVirtualTargetBinding& binding = calibration.targetBindings[index];
    if (binding.source == MappingVirtualTargetSource::RestFallback) {
        outTarget = targetFromTransform(mappingRoleForSlot(slot), calibration.trackerToTarget[index]);
        return true;
    }
    if (binding.source == MappingVirtualTargetSource::ParentedTracker) {
        return parentedPoseTarget(
            calibration,
            poses,
            originEnabled,
            originOffset,
            originRotationDegrees,
            slot,
            outTarget
        );
    }
    return mappedPoseTarget(
        calibration,
        poses,
        originEnabled,
        originOffset,
        originRotationDegrees,
        slot,
        outTarget
    );
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
        if (!buildTargetForSlot(
                calibration,
                poses,
                originEnabled,
                originOffset,
                originRotationDegrees,
                slot,
                outTargets[index]
            )) {
            return MappingVirtualTargetBuildResult{false, slot, true};
        }
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
        if (buildTargetForSlot(
                calibration,
                poses,
                originEnabled,
                originOffset,
                originRotationDegrees,
                slot,
                outTargets[index]
            )) {
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
