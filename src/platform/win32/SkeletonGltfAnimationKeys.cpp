#include "platform/win32/SkeletonGltfAnimationKeys.h"

#include "math/QuaternionUtils.h"

#include <array>
#include <cstddef>

namespace ovtr::win32 {
namespace {

float quaternionDot(const std::array<float, 4>& a, const std::array<float, 4>& b) noexcept
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}

void appendQuaternion(std::vector<float>& values, const std::array<float, 4>& q)
{
    values.insert(values.end(), q.begin(), q.end());
}

} // namespace

std::vector<float> continuousJointRotationKeys(const std::vector<SkeletonPose>& poses, const int joint)
{
    std::vector<float> values;
    values.reserve(poses.size() * 4U);
    std::array<float, 4> previous{0.0f, 0.0f, 0.0f, 1.0f};
    bool hasPrevious = false;
    for (const SkeletonPose& pose : poses) {
        std::array<float, 4> q = ovtr::normalizeQuaternion(
            pose.bones[static_cast<std::size_t>(joint)].localRotation
        );
        if (hasPrevious && quaternionDot(previous, q) < 0.0f) {
            for (float& component : q) {
                component = -component;
            }
        }
        appendQuaternion(values, q);
        previous = q;
        hasPrevious = true;
    }
    return values;
}

} // namespace ovtr::win32
