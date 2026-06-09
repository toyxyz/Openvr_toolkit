#include "TestCases.h"
#include "TestSupport.h"

#include "math/PoseTransform.h"
#include "platform/win32/MappingPinnedTargets.h"
#include "platform/win32/ProfileSkeleton.h"

#include <array>
#include <cmath>
#include <cstddef>

#ifdef _WIN32
namespace ovtr::test {
namespace {

float quatDot(const std::array<float, 4>& left, const std::array<float, 4>& right) noexcept
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2] + left[3] * right[3];
}

void requireSameRotation(
    const std::array<float, 4>& actual,
    const std::array<float, 4>& expected,
    const char* message
) {
    require(std::fabs(quatDot(actual, expected)) > 0.999f, message);
}

std::array<ovtr::win32::MappingVirtualTarget, ovtr::win32::kMappingSlotCount> makeTargets()
{
    std::array<ovtr::win32::MappingVirtualTarget, ovtr::win32::kMappingSlotCount> targets{};
    for (int slot = 0; slot < ovtr::win32::kMappingSlotCount; ++slot) {
        auto& target = targets[static_cast<std::size_t>(slot)];
        target.role = ovtr::win32::mappingRoleForSlot(slot);
        target.valid = true;
    }
    return targets;
}

} // namespace

void testWin32MappingPinnedTargets()
{
    ovtr::win32::BodyProfile profile;
    const ovtr::win32::ProfileSkeletonJoints rest = ovtr::win32::buildProfileSkeletonJoints(profile);
    ovtr::win32::SkeletonPose pose = ovtr::win32::makeRestSkeletonPose(rest);
    auto targets = makeTargets();
    const auto handRotation = ovtr::quaternionFromEulerDegrees({0.0f, 35.0f, 0.0f});
    const auto footRotation = ovtr::quaternionFromEulerDegrees({18.0f, 0.0f, 0.0f});
    targets[ovtr::win32::mappingSlotForRole(ovtr::win32::MappingTrackerRole::LeftHand)].transform.rotation = handRotation;
    targets[ovtr::win32::mappingSlotForRole(ovtr::win32::MappingTrackerRole::RightFoot)].transform.rotation = footRotation;

    ovtr::win32::MappingCalibrationData calibration;
    ovtr::win32::applyPinnedMappingTargets(calibration, rest, targets, pose);
    const auto pinnedRotations = ovtr::win32::computeSkeletonPoseWorldRotations(rest, pose);
    requireSameRotation(pinnedRotations[ovtr::win32::kProfileJointLeftHand], handRotation, "pinned hand follows target rotation");
    requireSameRotation(pinnedRotations[ovtr::win32::kProfileJointRightFoot], footRotation, "pinned foot follows target rotation");

    pose = ovtr::win32::makeRestSkeletonPose(rest);
    calibration.pinHandTargets = false;
    calibration.pinFootTargets = false;
    ovtr::win32::applyPinnedMappingTargets(calibration, rest, targets, pose);
    const auto unpinnedRotations = ovtr::win32::computeSkeletonPoseWorldRotations(rest, pose);
    require(std::fabs(quatDot(unpinnedRotations[ovtr::win32::kProfileJointLeftHand], handRotation)) < 0.999f, "disabled hand pin keeps existing rotation");
    require(std::fabs(quatDot(unpinnedRotations[ovtr::win32::kProfileJointRightFoot], footRotation)) < 0.999f, "disabled foot pin keeps existing rotation");
}

} // namespace ovtr::test
#endif
