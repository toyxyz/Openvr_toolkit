#include "vr/OpenVRProviderDetails.h"

#include "math/QuaternionUtils.h"

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>
#endif

#include <array>
#include <cmath>

namespace ovtr::openvr_provider_detail {

#ifdef OVTR_HAS_OPENVR_SDK
namespace {

std::array<float, 4> quaternionFromOpenVRMatrix(const vr::HmdMatrix34_t& matrix)
{
    const float m00 = matrix.m[0][0];
    const float m01 = matrix.m[0][1];
    const float m02 = matrix.m[0][2];
    const float m10 = matrix.m[1][0];
    const float m11 = matrix.m[1][1];
    const float m12 = matrix.m[1][2];
    const float m20 = matrix.m[2][0];
    const float m21 = matrix.m[2][1];
    const float m22 = matrix.m[2][2];

    std::array<float, 4> q{};
    const float trace = m00 + m11 + m22;
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        q[3] = 0.25f * s;
        q[0] = (m21 - m12) / s;
        q[1] = (m02 - m20) / s;
        q[2] = (m10 - m01) / s;
    } else if (m00 > m11 && m00 > m22) {
        const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
        q[3] = (m21 - m12) / s;
        q[0] = 0.25f * s;
        q[1] = (m01 + m10) / s;
        q[2] = (m02 + m20) / s;
    } else if (m11 > m22) {
        const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
        q[3] = (m02 - m20) / s;
        q[0] = (m01 + m10) / s;
        q[1] = 0.25f * s;
        q[2] = (m12 + m21) / s;
    } else {
        const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
        q[3] = (m10 - m01) / s;
        q[0] = (m02 + m20) / s;
        q[1] = (m12 + m21) / s;
        q[2] = 0.25f * s;
    }

    return normalizeQuaternion(q);
}

} // namespace

PoseSample makePoseSample(
    const vr::TrackedDeviceIndex_t index,
    const vr::TrackedDevicePose_t& trackedPose
)
{
    PoseSample pose;
    pose.deviceId = static_cast<DeviceId>(index + 1);
    pose.runtimeIndex = index;

    const vr::HmdMatrix34_t& matrix = trackedPose.mDeviceToAbsoluteTracking;
    pose.position = {matrix.m[0][3], matrix.m[1][3], matrix.m[2][3]};
    pose.rotation = quaternionFromOpenVRMatrix(matrix);
    pose.velocity = {
        trackedPose.vVelocity.v[0],
        trackedPose.vVelocity.v[1],
        trackedPose.vVelocity.v[2],
    };
    pose.angularVelocity = {
        trackedPose.vAngularVelocity.v[0],
        trackedPose.vAngularVelocity.v[1],
        trackedPose.vAngularVelocity.v[2],
    };

    pose.flags = 0;
    if (trackedPose.bDeviceIsConnected) {
        pose.flags |= PoseFlagDeviceConnected;
    }
    if (trackedPose.bPoseIsValid) {
        pose.flags |= PoseFlagPoseValid;
    }
    pose.flags |= PoseFlagRecordEnabled;
    return pose;
}
#endif

} // namespace ovtr::openvr_provider_detail
