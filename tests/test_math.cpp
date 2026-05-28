#include "TestCases.h"
#include "TestSupport.h"

#include "math/PoseInterpolation.h"
#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"

#include <array>
#include <cmath>
#include <vector>

namespace ovtr::test {
namespace {

struct TestInterpolationKey {
    double timeSeconds = 0.0;
};

} // namespace

void testPoseInterpolation()
{
    const std::array<float, 3> midpoint = ovtr::lerpVec3(
        std::array<float, 3>{1.0f, 2.0f, 3.0f},
        std::array<float, 3>{3.0f, 4.0f, 5.0f},
        0.5
    );
    require(std::fabs(midpoint[0] - 2.0f) < 0.0001f, "lerp x mismatch");
    require(std::fabs(midpoint[1] - 3.0f) < 0.0001f, "lerp y mismatch");
    require(std::fabs(midpoint[2] - 4.0f) < 0.0001f, "lerp z mismatch");

    const std::array<float, 4> halfway = ovtr::slerpQuaternion(
        axisAngleQuaternionZ(0.0),
        axisAngleQuaternionZ(180.0),
        0.5
    );
    const float halfSqrt = std::sqrt(0.5f);
    require(std::fabs(halfway[2] - halfSqrt) < 0.0001f, "slerp midpoint z mismatch");
    require(std::fabs(halfway[3] - halfSqrt) < 0.0001f, "slerp midpoint w mismatch");
    require(
        std::fabs(ovtr::quaternionDot({0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}) - 1.0) < 0.0001,
        "quaternion dot mismatch"
    );
}

void testPoseInterpolationSkipsUnreasonableResample()
{
    std::vector<TestInterpolationKey> keys{{0.0}, {2000.0}};
    ovtr::resampleKeyframesByTime(
        keys,
        1000.0,
        [](const TestInterpolationKey&, const TestInterpolationKey&, const double timeSeconds) {
            return TestInterpolationKey{timeSeconds};
        }
    );

    require(keys.size() == 2, "unreasonable resample should preserve original key count");
    require(keys.front().timeSeconds == 0.0, "unreasonable resample should preserve first key");
    require(keys.back().timeSeconds == 2000.0, "unreasonable resample should preserve last key");
}

void testPoseTransform()
{
    const std::array<float, 4> y90 = ovtr::quaternionFromEulerDegrees({0.0f, 90.0f, 0.0f});
    require(std::fabs(ovtr::yawDegreesFromQuaternion(y90) - 90.0f) < 0.01f, "yaw extraction mismatch");

    const std::array<float, 3> forward = ovtr::rotatePositionByQuaternion(y90, {0.0f, 0.0f, 1.0f});
    require(std::fabs(forward[0] - 1.0f) < 0.0001f, "quaternion vector rotate x mismatch");
    require(std::fabs(forward[1]) < 0.0001f, "quaternion vector rotate y mismatch");
    require(std::fabs(forward[2]) < 0.0001f, "quaternion vector rotate z mismatch");

    const std::array<float, 3> tinyQuaternionRotate =
        ovtr::rotatePositionByQuaternion({0.00000001f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    require(std::fabs(tinyQuaternionRotate[0]) < 0.0001f, "tiny quaternion should preserve x");
    require(std::fabs(tinyQuaternionRotate[1] - 1.0f) < 0.0001f, "tiny quaternion should preserve y");
    require(std::fabs(tinyQuaternionRotate[2]) < 0.0001f, "tiny quaternion should preserve z");

    ovtr::PoseSample pose;
    pose.runtimeIndex = 7;
    pose.position = {2.0f, 0.0f, 1.0f};
    pose.rotation = y90;
    pose.flags = ovtr::PoseFlagDeviceConnected | ovtr::PoseFlagPoseValid;

    const ovtr::PoseSample transformed =
        ovtr::applyOriginToPose(pose, true, {1.0f, 0.0f, 1.0f}, {0.0f, 90.0f, 0.0f});
    require(transformed.runtimeIndex == 7, "origin transform should preserve runtime index");
    require(transformed.flags == pose.flags, "origin transform should preserve flags");
    require(std::fabs(transformed.position[0]) < 0.0001f, "origin transform x mismatch");
    require(std::fabs(transformed.position[1]) < 0.0001f, "origin transform y mismatch");
    require(std::fabs(transformed.position[2] - 1.0f) < 0.0001f, "origin transform z mismatch");
    require(std::fabs(transformed.rotation[0]) < 0.0001f, "origin transform rotation x mismatch");
    require(std::fabs(transformed.rotation[1]) < 0.0001f, "origin transform rotation y mismatch");
    require(std::fabs(transformed.rotation[2]) < 0.0001f, "origin transform rotation z mismatch");
    require(std::fabs(transformed.rotation[3] - 1.0f) < 0.0001f, "origin transform rotation w mismatch");

    const ovtr::PoseSample unchanged =
        ovtr::applyOriginToPose(pose, false, {1.0f, 0.0f, 1.0f}, {0.0f, 90.0f, 0.0f});
    require(unchanged.position == pose.position, "disabled origin should preserve position");
    require(unchanged.rotation == pose.rotation, "disabled origin should preserve rotation");
}

} // namespace ovtr::test
