#include "platform/win32/MappingCalibrationCapture.h"

#include "math/PoseTransform.h"
#include "platform/win32/AppStateConstants.h"
#include "platform/win32/MappingCalibrationTargets.h"
#include "platform/win32/MappingTransformMath.h"
#include "platform/win32/SkeletonPose.h"

#include <array>
#include <cstddef>
#include <set>

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

std::wstring slotFailureMessage(const wchar_t* prefix, const int slotIndex)
{
    const auto& slots = mappingSlotDefinitions();
    return std::wstring(prefix) + slots[static_cast<std::size_t>(slotIndex)].label + L".";
}

MappingCalibrationStatus fail(const std::wstring& message)
{
    return MappingCalibrationStatus{false, message};
}

bool isMappedRuntimeIndex(const std::uint32_t runtimeIndex) noexcept
{
    return runtimeIndex != kNoSelectedRuntimeIndex;
}

int parentFootSlotFor(const int slot) noexcept
{
    const MappingTrackerRole role = mappingRoleForSlot(slot);
    if (role == MappingTrackerRole::LeftLeg) {
        return mappingSlotForRole(MappingTrackerRole::LeftFoot);
    }
    if (role == MappingTrackerRole::RightLeg) {
        return mappingSlotForRole(MappingTrackerRole::RightFoot);
    }
    return -1;
}

MappingTransform transformForMappedSlot(
    const std::array<std::uint32_t, kMappingSlotCount>& mappingRuntimeIndices,
    const ovtr::PosePollResult& poses,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees,
    const int slot
) {
    const ovtr::PoseSample* pose = poseForRuntimeIndex(
        poses,
        mappingRuntimeIndices[static_cast<std::size_t>(slot)]
    );
    const ovtr::PoseSample displayPose = ovtr::applyOriginToPose(
        *pose,
        originEnabled,
        originOffset,
        originRotationDegrees
    );
    return mappingTransformFromPose(displayPose);
}

} // namespace

MappingCalibrationStatus captureMappingActorCalibration(
    MappingActor& actor,
    const std::array<std::uint32_t, kMappingSlotCount>& mappingRuntimeIndices,
    const ovtr::PosePollResult& poses,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees,
    const float armSoftIkStrength,
    const float legSoftIkStrength
) {
    std::set<std::uint32_t> usedRuntimeIndices;
    bool hasMappedSlot = false;
    for (int slot = 0; slot < kMappingSlotCount; ++slot) {
        const std::uint32_t runtimeIndex = mappingRuntimeIndices[static_cast<std::size_t>(slot)];
        if (!isMappedRuntimeIndex(runtimeIndex)) {
            continue;
        }
        hasMappedSlot = true;
        if (!usedRuntimeIndices.insert(runtimeIndex).second) {
            return fail(slotFailureMessage(L"Mapping slot duplicates a device: ", slot));
        }
        const ovtr::PoseSample* pose = poseForRuntimeIndex(poses, runtimeIndex);
        if (!pose || (pose->flags & ovtr::PoseFlagPoseValid) == 0) {
            return fail(slotFailureMessage(L"Mapping slot has no valid pose: ", slot));
        }
    }
    if (!hasMappedSlot) {
        return fail(L"Mapping calibration requires at least one mapped tracker.");
    }

    const auto restTargets = mappingCalibrationRestTargets(actor.profile);
    std::array<MappingTransform, kMappingSlotCount> offsets{};
    std::array<MappingVirtualTargetBinding, kMappingSlotCount> bindings{};
    std::array<MappingVirtualTarget, kMappingSlotCount> liveTargets{};
    for (int slot = 0; slot < kMappingSlotCount; ++slot) {
        const std::size_t index = static_cast<std::size_t>(slot);
        offsets[index] = restTargets[index];
        bindings[index] = MappingVirtualTargetBinding{MappingVirtualTargetSource::RestFallback, -1};
        liveTargets[index] = MappingVirtualTarget{
            mappingRoleForSlot(slot),
            restTargets[index],
            {},
            true
        };
        if (!isMappedRuntimeIndex(mappingRuntimeIndices[index])) {
            const int parentSlot = parentFootSlotFor(slot);
            if (parentSlot < 0 || !isMappedRuntimeIndex(mappingRuntimeIndices[static_cast<std::size_t>(parentSlot)])) {
                continue;
            }
            const MappingTransform parentTracker = transformForMappedSlot(
                mappingRuntimeIndices,
                poses,
                originEnabled,
                originOffset,
                originRotationDegrees,
                parentSlot
            );
            offsets[index] = composeMappingTransforms(
                inverseMappingTransform(parentTracker),
                restTargets[index]
            );
            bindings[index] = MappingVirtualTargetBinding{
                MappingVirtualTargetSource::ParentedTracker,
                parentSlot
            };
            continue;
        }
        const MappingTransform tracker = transformForMappedSlot(
            mappingRuntimeIndices,
            poses,
            originEnabled,
            originOffset,
            originRotationDegrees,
            slot
        );
        offsets[index] = composeMappingTransforms(
            inverseMappingTransform(tracker),
            restTargets[index]
        );
        bindings[index] = MappingVirtualTargetBinding{MappingVirtualTargetSource::DirectTracker, -1};
        liveTargets[index] = MappingVirtualTarget{
            mappingRoleForSlot(slot),
            restTargets[index],
            tracker,
            true
        };
    }

    actor.calibrated = true;
    actor.mappingDeviceRuntimeIndices = mappingRuntimeIndices;
    actor.calibration.runtimeIndices = mappingRuntimeIndices;
    actor.calibration.trackerToTarget = offsets;
    actor.calibration.targetBindings = bindings;
    actor.calibration.armSoftIkStrength = armSoftIkStrength;
    actor.calibration.legSoftIkStrength = legSoftIkStrength;
    actor.liveJoints = buildProfileSkeletonJoints(actor.profile);
    actor.liveSkeletonPose = makeRestSkeletonPose(actor.liveJoints);
    actor.liveJointsValid = true;
    actor.liveTrackingLost = false;
    actor.liveLimited = false;
    actor.liveVirtualTargets = liveTargets;
    actor.liveDebugPoles = {};
    actor.livePoleDirections = {};
    actor.livePoleDirectionValid = {};
    actor.livePoleTargets = {};
    actor.livePoleTargetValid = {};
    actor.liveFingerJointsValid = {};
    return MappingCalibrationStatus{true, L"Calibration complete."};
}

} // namespace ovtr::win32
