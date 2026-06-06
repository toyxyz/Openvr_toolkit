#pragma once

#include "platform/win32/ProfileSkeleton.h"

#include <algorithm>

namespace ovtr::win32 {

struct BodyBoneBoxStyle {
    float halfSide = 0.02f;
    float halfDepth = 0.02f;
};

inline BodyBoneBoxStyle scaledBodyBoneBoxStyle(
    const float heightMeters,
    const float sideMultiplier,
    const float depthMultiplier,
    const float minHalf,
    const float maxHalf
) noexcept {
    return {
        std::clamp(heightMeters * sideMultiplier, minHalf, maxHalf),
        std::clamp(heightMeters * depthMultiplier, minHalf, maxHalf)
    };
}

inline BodyBoneBoxStyle styleForBodyBone(const int childIndex, const float heightMeters) noexcept
{
    if (isProfileFingerJoint(childIndex)) {
        return scaledBodyBoneBoxStyle(heightMeters, 0.005f, 0.004f, 0.004f, 0.012f);
    }
    switch (childIndex) {
    case kProfileJointSpine:
    case kProfileJointSpine1:
        return scaledBodyBoneBoxStyle(heightMeters, 0.030f, 0.022f, 0.030f, 0.060f);
    case kProfileJointSpine2:
        return scaledBodyBoneBoxStyle(heightMeters, 0.052f, 0.038f, 0.060f, 0.100f);
    case kProfileJointNeck:
        return scaledBodyBoneBoxStyle(heightMeters, 0.010f, 0.010f, 0.014f, 0.024f);
    case kProfileJointHead:
    case kProfileJointHeadTopEnd:
        return scaledBodyBoneBoxStyle(heightMeters, 0.034f, 0.034f, 0.045f, 0.070f);
    case kProfileJointLeftShoulder:
    case kProfileJointRightShoulder:
        return scaledBodyBoneBoxStyle(heightMeters, 0.010f, 0.008f, 0.012f, 0.022f);
    case kProfileJointLeftArm:
    case kProfileJointRightArm:
        return scaledBodyBoneBoxStyle(heightMeters, 0.017f, 0.014f, 0.020f, 0.034f);
    case kProfileJointLeftForeArm:
    case kProfileJointRightForeArm:
        return scaledBodyBoneBoxStyle(heightMeters, 0.014f, 0.012f, 0.018f, 0.030f);
    case kProfileJointLeftHand:
    case kProfileJointRightHand:
        return scaledBodyBoneBoxStyle(heightMeters, 0.018f, 0.006f, 0.010f, 0.035f);
    case kProfileJointLeftUpLeg:
    case kProfileJointRightUpLeg:
    case kProfileJointLeftLeg:
    case kProfileJointRightLeg:
        return scaledBodyBoneBoxStyle(heightMeters, 0.018f, 0.015f, 0.022f, 0.040f);
    case kProfileJointLeftFoot:
    case kProfileJointRightFoot:
        return scaledBodyBoneBoxStyle(heightMeters, 0.014f, 0.012f, 0.018f, 0.032f);
    case kProfileJointLeftToeBase:
    case kProfileJointRightToeBase:
        return scaledBodyBoneBoxStyle(heightMeters, 0.024f, 0.007f, 0.012f, 0.045f);
    default:
        return scaledBodyBoneBoxStyle(heightMeters, 0.016f, 0.014f, 0.018f, 0.034f);
    }
}

} // namespace ovtr::win32
