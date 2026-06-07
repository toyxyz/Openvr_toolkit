#include "platform/win32/VmcLimbSpacing.h"

#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"

#include <cmath>
#include <cstddef>

namespace ovtr::win32 {
namespace {

float clampSpacingDegrees(const float degrees) noexcept
{
    if (!std::isfinite(degrees)) {
        return 0.0f;
    }
    if (degrees < -180.0f) {
        return -180.0f;
    }
    if (degrees > 180.0f) {
        return 180.0f;
    }
    return degrees;
}

std::array<float, 4> zRotationDegrees(const float degrees) noexcept
{
    return ovtr::quaternionFromEulerDegrees({0.0f, 0.0f, degrees});
}

void applyLocalZSpacing(
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& localRotations,
    const int joint,
    const float degrees
) noexcept {
    if (std::fabs(degrees) <= 0.0001f) {
        return;
    }
    auto& local = localRotations[static_cast<std::size_t>(joint)];
    local = ovtr::normalizeQuaternion(ovtr::multiplyQuaternion(local, zRotationDegrees(degrees)));
}

} // namespace

void applyVmcLimbSpacing(
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& localRotations,
    const float armSpacingDegrees,
    const float legSpacingDegrees
) noexcept {
    const float arm = clampSpacingDegrees(armSpacingDegrees);
    const float leg = clampSpacingDegrees(legSpacingDegrees);
    applyLocalZSpacing(localRotations, kProfileJointLeftArm, arm);
    applyLocalZSpacing(localRotations, kProfileJointRightArm, -arm);
    applyLocalZSpacing(localRotations, kProfileJointLeftUpLeg, leg);
    applyLocalZSpacing(localRotations, kProfileJointRightUpLeg, -leg);
}

} // namespace ovtr::win32
