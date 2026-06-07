#include "TestCases.h"
#include "TestSupport.h"

#ifdef _WIN32
#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonPose.h"
#include "platform/win32/VmcLegRotationContinuity.h"
#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"

#include <array>
#include <cmath>

namespace {

float quaternionAbsDot(const std::array<float, 4>& a, const std::array<float, 4>& b)
{
    return std::fabs(a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]);
}

std::array<float, 4> negated(std::array<float, 4> q)
{
    for (float& value : q) {
        value = -value;
    }
    return q;
}

std::array<float, 4> forearmRollAlternate(const std::array<float, 4>& q)
{
    constexpr std::array<float, 4> kLocalYHalfTurn{0.0f, 1.0f, 0.0f, 0.0f};
    return ovtr::normalizeQuaternion(ovtr::multiplyQuaternion(q, kLocalYHalfTurn));
}

} // namespace

namespace ovtr::test {

void testWin32VmcArmContinuity()
{
    ovtr::win32::BodyProfile profile;
    const ovtr::win32::ProfileSkeletonJoints rest =
        ovtr::win32::buildProfileSkeletonJoints(profile);
    ovtr::win32::ProfileSkeletonJoints poseJoints = rest;
    poseJoints[ovtr::win32::kProfileJointRightArm].positionMeters.y -= 0.08f;
    poseJoints[ovtr::win32::kProfileJointRightForeArm].positionMeters.z -= 0.16f;
    const ovtr::win32::SkeletonPose armPose =
        ovtr::win32::makeSkeletonPoseFromWorldJoints(rest, poseJoints);

    ovtr::win32::VmcLegRotationContinuity freshContinuity;
    const auto freshRotations =
        ovtr::win32::makeVmcLocalRotationsWithContinuity(rest, poseJoints, armPose, freshContinuity);
    require(
        quaternionAbsDot(
            freshRotations[ovtr::win32::kProfileJointRightArm],
            armPose.bones[ovtr::win32::kProfileJointRightArm].localRotation
        ) > 0.99f,
        "VMC arm continuity keeps raw upper arm direction"
    );

    const auto world = ovtr::win32::computeSkeletonPoseWorldRotations(rest, armPose);
    require(
        quaternionAbsDot(
            freshContinuity.previousWorld[ovtr::win32::kProfileJointRightForeArm],
            world[ovtr::win32::kProfileJointRightForeArm]
        ) > 0.99f,
        "VMC forearm initial roll basin uses raw SkeletonPose rotation"
    );

    ovtr::win32::VmcLegRotationContinuity signContinuity;
    signContinuity.valid[ovtr::win32::kProfileJointRightArm] = true;
    signContinuity.previousWorld[ovtr::win32::kProfileJointRightArm] =
        negated(world[ovtr::win32::kProfileJointRightArm]);
    const auto signRotations =
        ovtr::win32::makeVmcLocalRotationsWithContinuity(rest, poseJoints, armPose, signContinuity);
    require(
        quaternionAbsDot(
            signRotations[ovtr::win32::kProfileJointRightArm],
            freshRotations[ovtr::win32::kProfileJointRightArm]
        ) > 0.99f,
        "VMC arm continuity only changes quaternion hemisphere"
    );

    ovtr::win32::SkeletonPose twistedPose = armPose;
    twistedPose.bones[ovtr::win32::kProfileJointRightForeArm].localRotation =
        forearmRollAlternate(armPose.bones[ovtr::win32::kProfileJointRightForeArm].localRotation);
    ovtr::win32::VmcLegRotationContinuity twistedContinuity;
    ovtr::win32::makeVmcLocalRotationsWithContinuity(rest, poseJoints, twistedPose, twistedContinuity);
    require(
        quaternionAbsDot(
            twistedContinuity.previousWorld[ovtr::win32::kProfileJointRightForeArm],
            freshContinuity.previousWorld[ovtr::win32::kProfileJointRightForeArm]
        ) > 0.99f,
        "VMC forearm first frame uses rest roll sign instead of raw twisted roll"
    );
}

} // namespace ovtr::test
#endif
