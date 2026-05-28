#include "math/PoseTransform.h"

#include <cmath>

namespace ovtr {
namespace {

constexpr float kPi = 3.14159265359f;

std::array<float, 4> normalizeTransformQuaternion(const std::array<float, 4>& quaternion)
{
    const float length = std::sqrt(
        quaternion[0] * quaternion[0] +
        quaternion[1] * quaternion[1] +
        quaternion[2] * quaternion[2] +
        quaternion[3] * quaternion[3]
    );
    if (length <= 0.000001f) {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }
    return {
        quaternion[0] / length,
        quaternion[1] / length,
        quaternion[2] / length,
        quaternion[3] / length,
    };
}

} // namespace

std::array<float, 4> multiplyQuaternion(
    const std::array<float, 4>& left,
    const std::array<float, 4>& right
)
{
    const float lx = left[0];
    const float ly = left[1];
    const float lz = left[2];
    const float lw = left[3];
    const float rx = right[0];
    const float ry = right[1];
    const float rz = right[2];
    const float rw = right[3];
    return {
        lw * rx + lx * rw + ly * rz - lz * ry,
        lw * ry - lx * rz + ly * rw + lz * rx,
        lw * rz + lx * ry - ly * rx + lz * rw,
        lw * rw - lx * rx - ly * ry - lz * rz,
    };
}

std::array<float, 4> conjugateQuaternion(const std::array<float, 4>& quaternion)
{
    return {-quaternion[0], -quaternion[1], -quaternion[2], quaternion[3]};
}

std::array<float, 4> quaternionFromEulerDegrees(const std::array<float, 3>& degrees)
{
    const float halfX = degrees[0] * kPi / 360.0f;
    const float halfY = degrees[1] * kPi / 360.0f;
    const float halfZ = degrees[2] * kPi / 360.0f;

    const std::array<float, 4> qx{std::sin(halfX), 0.0f, 0.0f, std::cos(halfX)};
    const std::array<float, 4> qy{0.0f, std::sin(halfY), 0.0f, std::cos(halfY)};
    const std::array<float, 4> qz{0.0f, 0.0f, std::sin(halfZ), std::cos(halfZ)};
    return normalizeTransformQuaternion(multiplyQuaternion(qz, multiplyQuaternion(qy, qx)));
}

std::array<float, 3> rotatePositionByQuaternion(
    const std::array<float, 4>& quaternion,
    const std::array<float, 3>& position
)
{
    const std::array<float, 4> vectorQuaternion{position[0], position[1], position[2], 0.0f};
    const std::array<float, 4> normalized = normalizeTransformQuaternion(quaternion);
    const std::array<float, 4> rotated = multiplyQuaternion(
        multiplyQuaternion(normalized, vectorQuaternion),
        conjugateQuaternion(normalized)
    );
    return {rotated[0], rotated[1], rotated[2]};
}

float yawDegreesFromQuaternion(const std::array<float, 4>& quaternion)
{
    const std::array<float, 3> forward = rotatePositionByQuaternion(quaternion, {0.0f, 0.0f, 1.0f});
    if (std::sqrt(forward[0] * forward[0] + forward[2] * forward[2]) <= 0.000001f) {
        return 0.0f;
    }
    return std::atan2(forward[0], forward[2]) * 180.0f / kPi;
}

} // namespace ovtr
