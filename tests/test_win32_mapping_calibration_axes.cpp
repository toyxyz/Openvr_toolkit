#include "TestCases.h"

#ifdef _WIN32
#include "data/SessionTypes.h"
#include "math/PoseTransform.h"
#include "platform/win32/AppProfileState.h"
#include "platform/win32/MappingCalibrationCapture.h"
#include "platform/win32/MappingCalibrationSolve.h"
#include "platform/win32/MappingCalibrationTargets.h"
#include "platform/win32/MappingTransformMath.h"
#include "platform/win32/SkeletonPose.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace {

std::array<std::uint32_t, ovtr::win32::kMappingSlotCount> sequentialMapping()
{
    std::array<std::uint32_t, ovtr::win32::kMappingSlotCount> mapping{};
    for (std::size_t index = 0; index < mapping.size(); ++index) {
        mapping[index] = static_cast<std::uint32_t>(index + 1u);
    }
    return mapping;
}

ovtr::PosePollResult posesForTargets(
    const std::array<ovtr::win32::MappingTransform, ovtr::win32::kMappingSlotCount>& targets,
    const std::array<std::uint32_t, ovtr::win32::kMappingSlotCount>& mapping
) {
    ovtr::PosePollResult result;
    for (std::size_t index = 0; index < targets.size(); ++index) {
        ovtr::PoseSample pose;
        pose.runtimeIndex = mapping[index];
        pose.position = {targets[index].position.x, targets[index].position.y, targets[index].position.z};
        pose.rotation = targets[index].rotation;
        pose.flags = ovtr::PoseFlagDeviceConnected | ovtr::PoseFlagPoseValid;
        result.poses.push_back(pose);
    }
    return result;
}

ovtr::win32::Vec3 cross(ovtr::win32::Vec3 left, ovtr::win32::Vec3 right) noexcept
{
    return {
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x,
    };
}

ovtr::win32::Vec3 limbPlaneNormal(
    const ovtr::win32::ProfileSkeletonJoints& joints,
    ovtr::win32::ProfileSkeletonJointIndex root,
    ovtr::win32::ProfileSkeletonJointIndex mid,
    ovtr::win32::ProfileSkeletonJointIndex end,
    ovtr::win32::Vec3 fallback
) {
    using namespace ovtr::win32;
    const Vec3 upper = subMappingVec3(joints[mid].positionMeters, joints[root].positionMeters);
    const Vec3 lower = subMappingVec3(joints[end].positionMeters, joints[mid].positionMeters);
    return normalizeMappingVec3Or(cross(upper, lower), fallback);
}

void requireAxisNear(ovtr::win32::Vec3 actual, ovtr::win32::Vec3 expected, const char* message)
{
    using namespace ovtr::win32;
    actual = normalizeMappingVec3Or(actual, {1.0f, 0.0f, 0.0f});
    expected = normalizeMappingVec3Or(expected, {1.0f, 0.0f, 0.0f});
    const float dot = dotMappingVec3(actual, expected);
    if (dot < 0.95f) {
        throw std::runtime_error(
            std::string(message) + " actual=(" +
            std::to_string(actual.x) + "," + std::to_string(actual.y) + "," + std::to_string(actual.z) +
            ") expected=(" + std::to_string(expected.x) + "," + std::to_string(expected.y) + "," +
            std::to_string(expected.z) + ") dot=" + std::to_string(dot)
        );
    }
}

ovtr::win32::Vec3 profileJointSideAxis(const ovtr::win32::ProfileSkeletonJoints& joints, int joint);
ovtr::win32::Vec3 profileSegmentSideAxis(
    const ovtr::win32::ProfileSkeletonJoints& positions,
    const ovtr::win32::ProfileSkeletonJoints& hints,
    int segment
);

int firstChildIndex(const ovtr::win32::ProfileSkeletonJoints& joints, const int parentIndex) noexcept
{
    for (std::size_t index = 0; index < joints.size(); ++index) {
        if (joints[index].parentIndex == parentIndex) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

ovtr::win32::Vec3 jointMainAxis(
    const ovtr::win32::ProfileSkeletonJoints& joints,
    const int joint
) {
    using namespace ovtr::win32;
    const int child = firstChildIndex(joints, joint);
    if (child >= 0) {
        return normalizeMappingVec3Or(
            subMappingVec3(joints[child].positionMeters, joints[joint].positionMeters),
            {0.0f, 1.0f, 0.0f}
        );
    }
    const int parent = joints[joint].parentIndex;
    return parent >= 0
        ? normalizeMappingVec3Or(subMappingVec3(joints[joint].positionMeters, joints[parent].positionMeters), {0.0f, 1.0f, 0.0f})
        : ovtr::win32::Vec3{0.0f, 1.0f, 0.0f};
}

void requireLimbAxes(
    const ovtr::win32::ProfileSkeletonJoints& joints,
    const ovtr::win32::ProfileSkeletonJoints& rest,
    const std::array<ovtr::win32::Vec3, ovtr::win32::kProfileSkeletonJointCount>& sideAxes
)
{
    using namespace ovtr::win32;
    const Vec3 leftLeg = limbPlaneNormal(joints, kProfileJointLeftUpLeg, kProfileJointLeftLeg, kProfileJointLeftFoot, {1.0f, 0.0f, 0.0f});
    const Vec3 rightLeg = limbPlaneNormal(joints, kProfileJointRightUpLeg, kProfileJointRightLeg, kProfileJointRightFoot, {1.0f, 0.0f, 0.0f});
    requireAxisNear(sideAxes[kProfileJointLeftArm], profileSegmentSideAxis(joints, rest, kProfileJointLeftArm), "left upper-arm box axis should stay near profile roll");
    requireAxisNear(sideAxes[kProfileJointLeftForeArm], profileSegmentSideAxis(joints, rest, kProfileJointLeftForeArm), "left forearm box axis should stay near profile roll");
    requireAxisNear(sideAxes[kProfileJointRightArm], profileSegmentSideAxis(joints, rest, kProfileJointRightArm), "right upper-arm box axis should stay near profile roll");
    requireAxisNear(sideAxes[kProfileJointRightForeArm], profileSegmentSideAxis(joints, rest, kProfileJointRightForeArm), "right forearm box axis should stay near profile roll");
    requireAxisNear(sideAxes[kProfileJointLeftLeg], leftLeg, "left lower-leg box axis should follow leg plane");
    requireAxisNear(sideAxes[kProfileJointRightLeg], rightLeg, "right lower-leg box axis should follow leg plane");
}

ovtr::win32::Vec3 renderedJointSideAxis(
    const ovtr::win32::ProfileSkeletonJoints& joints,
    const std::array<ovtr::win32::Vec3, ovtr::win32::kProfileSkeletonJointCount>& sideAxes,
    const std::array<ovtr::win32::Vec3, ovtr::win32::kProfileSkeletonJointCount>& forwardAxes,
    const int joint
) {
    using namespace ovtr::win32;
    const int child = firstChildIndex(joints, joint);
    const int segment = child >= 0 ? child : joint;
    const Vec3 y = jointMainAxis(joints, joint);
    Vec3 x = sideAxes[static_cast<std::size_t>(segment)];
    x = subMappingVec3(x, scaleMappingVec3(y, dotMappingVec3(x, y)));
    return normalizeMappingVec3Or(
        x,
        normalizeMappingVec3Or(cross(y, forwardAxes[static_cast<std::size_t>(segment)]), {1.0f, 0.0f, 0.0f})
    );
}

ovtr::win32::Vec3 profileJointSideAxis(const ovtr::win32::ProfileSkeletonJoints& joints, const int joint)
{
    using namespace ovtr::win32;
    const int child = firstChildIndex(joints, joint);
    const int segment = child >= 0 ? child : joint;
    const Vec3 y = jointMainAxis(joints, joint);
    Vec3 x = joints[static_cast<std::size_t>(segment)].sideHint;
    x = subMappingVec3(x, scaleMappingVec3(y, dotMappingVec3(x, y)));
    const Vec3 fallback = std::fabs(y.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{0.0f, 0.0f, 1.0f};
    return normalizeMappingVec3Or(x, normalizeMappingVec3Or(cross(y, fallback), {1.0f, 0.0f, 0.0f}));
}

ovtr::win32::Vec3 profileSegmentSideAxis(
    const ovtr::win32::ProfileSkeletonJoints& positions,
    const ovtr::win32::ProfileSkeletonJoints& hints,
    const int segment
) {
    using namespace ovtr::win32;
    const int parent = positions[static_cast<std::size_t>(segment)].parentIndex;
    const Vec3 y = parent >= 0
        ? normalizeMappingVec3Or(subMappingVec3(positions[segment].positionMeters, positions[parent].positionMeters), {0.0f, 1.0f, 0.0f})
        : Vec3{0.0f, 1.0f, 0.0f};
    Vec3 x = hints[static_cast<std::size_t>(segment)].sideHint;
    x = subMappingVec3(x, scaleMappingVec3(y, dotMappingVec3(x, y)));
    const Vec3 fallback = std::fabs(y.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{0.0f, 0.0f, 1.0f};
    return normalizeMappingVec3Or(x, normalizeMappingVec3Or(cross(y, fallback), {1.0f, 0.0f, 0.0f}));
}

} // namespace

namespace ovtr::test {

void testWin32MappingCalibrationAxes()
{
    using namespace ovtr::win32;

    MappingActor actor;
    const auto mapping = sequentialMapping();
    auto targets = mappingCalibrationRestTargets(actor.profile);
    targets[static_cast<std::size_t>(mappingSlotForRole(MappingTrackerRole::LeftFoot))].rotation =
        ovtr::quaternionFromEulerDegrees({0.0f, 180.0f, 0.0f});
    targets[static_cast<std::size_t>(mappingSlotForRole(MappingTrackerRole::RightFoot))].rotation =
        ovtr::quaternionFromEulerDegrees({0.0f, 180.0f, 0.0f});
    const ovtr::PosePollResult poses = posesForTargets(targets, mapping);
    const MappingCalibrationStatus status = captureMappingActorCalibration(actor, mapping, poses, false, {}, {});
    if (!status.success || !updateCalibratedMappingActorJoints(actor, poses, false, {}, {})) {
        throw std::runtime_error("rest calibration should produce a live calibrated actor");
    }

    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(actor.profile);
    const auto liveSide = computeSkeletonPoseWorldSideAxes(rest, actor.liveSkeletonPose);
    const auto liveForward = computeSkeletonPoseWorldForwardAxes(rest, actor.liveSkeletonPose);
    const Vec3 leftArmRest = profileSegmentSideAxis(actor.liveJoints, rest, kProfileJointLeftArm);
    const Vec3 rightArmRest = profileSegmentSideAxis(actor.liveJoints, rest, kProfileJointRightArm);
    const Vec3 leftLegRest = limbPlaneNormal(rest, kProfileJointLeftUpLeg, kProfileJointLeftLeg, kProfileJointLeftFoot, {1.0f, 0.0f, 0.0f});
    const Vec3 rightLegRest = limbPlaneNormal(rest, kProfileJointRightUpLeg, kProfileJointRightLeg, kProfileJointRightFoot, {1.0f, 0.0f, 0.0f});

    requireAxisNear(renderedJointSideAxis(actor.liveJoints, liveSide, liveForward, kProfileJointLeftShoulder), leftArmRest, "left shoulder joint axis should not flip after calibration");
    requireAxisNear(renderedJointSideAxis(actor.liveJoints, liveSide, liveForward, kProfileJointRightShoulder), rightArmRest, "right shoulder joint axis should not flip after calibration");
    requireAxisNear(renderedJointSideAxis(actor.liveJoints, liveSide, liveForward, kProfileJointLeftUpLeg), leftLegRest, "left hip joint axis should not flip after calibration");
    requireAxisNear(renderedJointSideAxis(actor.liveJoints, liveSide, liveForward, kProfileJointRightUpLeg), rightLegRest, "right hip joint axis should not flip after calibration");
    requireAxisNear(renderedJointSideAxis(actor.liveJoints, liveSide, liveForward, kProfileJointLeftFoot), profileJointSideAxis(rest, kProfileJointLeftFoot), "left foot debug joint axis should match profile axis after calibration");
    requireAxisNear(renderedJointSideAxis(actor.liveJoints, liveSide, liveForward, kProfileJointRightFoot), profileJointSideAxis(rest, kProfileJointRightFoot), "right foot debug joint axis should match profile axis after calibration");
    requireAxisNear(jointMainAxis(actor.liveJoints, kProfileJointLeftFoot), jointMainAxis(rest, kProfileJointLeftFoot), "left foot-to-toe axis should not flip after calibration");
    requireAxisNear(jointMainAxis(actor.liveJoints, kProfileJointRightFoot), jointMainAxis(rest, kProfileJointRightFoot), "right foot-to-toe axis should not flip after calibration");

    requireLimbAxes(actor.liveJoints, rest, liveSide);
    for (const int joint : {
        kProfileJointHips,
        kProfileJointSpine,
        kProfileJointSpine1,
        kProfileJointSpine2,
        kProfileJointNeck,
        kProfileJointHeadTopEnd,
        kProfileJointLeftHand,
        kProfileJointRightHand,
    }) {
        requireAxisNear(liveSide[joint], buildProfileSkeletonJoints(actor.profile)[joint].sideHint, "body endpoint box axis should not rotate after rest calibration");
    }
    requireAxisNear(liveSide[kProfileJointLeftFoot], leftLegRest, "left foot box axis should preserve pre-calibration leg-plane axis");
    requireAxisNear(liveSide[kProfileJointRightFoot], rightLegRest, "right foot box axis should preserve pre-calibration leg-plane axis");
    requireAxisNear(liveSide[kProfileJointLeftToeBase], profileJointSideAxis(rest, kProfileJointLeftToeBase), "left toe box axis should match profile fallback axis");
    requireAxisNear(liveSide[kProfileJointRightToeBase], profileJointSideAxis(rest, kProfileJointRightToeBase), "right toe box axis should match profile fallback axis");

    auto loweredTargets = mappingCalibrationRestTargets(actor.profile);
    loweredTargets[static_cast<std::size_t>(mappingSlotForRole(MappingTrackerRole::LeftArm))].position =
        {rest[kProfileJointLeftArm].positionMeters.x, 1.15f, -0.20f};
    loweredTargets[static_cast<std::size_t>(mappingSlotForRole(MappingTrackerRole::LeftHand))].position =
        {rest[kProfileJointLeftForeArm].positionMeters.x - 0.20f, 1.05f, 0.0f};
    loweredTargets[static_cast<std::size_t>(mappingSlotForRole(MappingTrackerRole::RightArm))].position =
        {rest[kProfileJointRightArm].positionMeters.x, 1.15f, -0.20f};
    loweredTargets[static_cast<std::size_t>(mappingSlotForRole(MappingTrackerRole::RightHand))].position =
        {rest[kProfileJointRightForeArm].positionMeters.x + 0.20f, 1.05f, 0.0f};
    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(loweredTargets, mapping), false, {}, {})) {
        throw std::runtime_error("lowered-arm live solve should update");
    }
    const auto loweredSide = computeSkeletonPoseWorldSideAxes(rest, actor.liveSkeletonPose);
    loweredTargets[static_cast<std::size_t>(mappingSlotForRole(MappingTrackerRole::LeftHand))].position.z += 0.05f;
    loweredTargets[static_cast<std::size_t>(mappingSlotForRole(MappingTrackerRole::RightHand))].position.z += 0.05f;
    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(loweredTargets, mapping), false, {}, {})) {
        throw std::runtime_error("moved lowered-arm live solve should update");
    }
    const auto movedLoweredSide = computeSkeletonPoseWorldSideAxes(rest, actor.liveSkeletonPose);
    requireAxisNear(movedLoweredSide[kProfileJointLeftArm], loweredSide[kProfileJointLeftArm], "left upper-arm lowered roll should not flip");
    requireAxisNear(movedLoweredSide[kProfileJointLeftForeArm], loweredSide[kProfileJointLeftForeArm], "left forearm lowered roll should not flip");
    requireAxisNear(movedLoweredSide[kProfileJointRightArm], loweredSide[kProfileJointRightArm], "right upper-arm lowered roll should not flip");
    requireAxisNear(movedLoweredSide[kProfileJointRightForeArm], loweredSide[kProfileJointRightForeArm], "right forearm lowered roll should not flip");
}

} // namespace ovtr::test
#endif
