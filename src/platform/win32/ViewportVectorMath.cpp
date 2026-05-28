#include "platform/win32/ViewportMath.h"

#include "platform/win32/ViewportMathInternal.h"

#include <cmath>

namespace ovtr::win32 {

float clampFloat(const float value, const float minValue, const float maxValue) noexcept
{
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

float positiveCameraDistance(const float distance, const float minimumDistance) noexcept
{
    return distance < minimumDistance ? minimumDistance : distance;
}

Vec3 normalizeVec3(const Vec3 value) noexcept
{
    const float length = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    if (length <= 0.000001f) {
        return {};
    }
    return {value.x / length, value.y / length, value.z / length};
}

Vec3 rotateByInverseViewRotation(
    const float yawDegrees,
    const float pitchDegrees,
    const Vec3 cameraVector
) noexcept
{
    const float yaw = degreesToRadians(yawDegrees);
    const float pitch = degreesToRadians(pitchDegrees);
    const float cosPitch = std::cos(-pitch);
    const float sinPitch = std::sin(-pitch);
    const float cosYaw = std::cos(-yaw);
    const float sinYaw = std::sin(-yaw);

    const Vec3 afterPitch{
        cameraVector.x,
        cameraVector.y * cosPitch - cameraVector.z * sinPitch,
        cameraVector.y * sinPitch + cameraVector.z * cosPitch,
    };

    return normalizeVec3({
        afterPitch.x * cosYaw + afterPitch.z * sinYaw,
        afterPitch.y,
        -afterPitch.x * sinYaw + afterPitch.z * cosYaw,
    });
}

} // namespace ovtr::win32
