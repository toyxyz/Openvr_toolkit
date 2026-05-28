#include "platform/win32/ViewportMath.h"

#include "platform/win32/ViewportMathInternal.h"

#include <cmath>

namespace ovtr::win32 {
namespace {

constexpr float kPanScalePerPixel = 0.0018f;
constexpr float kMinimumDepth = 0.001f;

} // namespace

Vec3 screenSpacePanOffset(
    const CameraView& view,
    const int dx,
    const int dy,
    const float minimumCameraDistance
) noexcept
{
    const Vec3 right = rotateByInverseViewRotation(view.yawDegrees, view.pitchDegrees, {1.0f, 0.0f, 0.0f});
    const Vec3 up = rotateByInverseViewRotation(view.yawDegrees, view.pitchDegrees, {0.0f, 1.0f, 0.0f});

    const float panScale = kPanScalePerPixel * positiveCameraDistance(view.distance, minimumCameraDistance);
    const float moveRight = -static_cast<float>(dx) * panScale;
    const float moveUp = static_cast<float>(dy) * panScale;

    return {
        right.x * moveRight + up.x * moveUp,
        right.y * moveRight + up.y * moveUp,
        right.z * moveRight + up.z * moveUp,
    };
}

Vec3 cameraDollyOffset(const CameraView& view, const float distance) noexcept
{
    const Vec3 forward = rotateByInverseViewRotation(view.yawDegrees, view.pitchDegrees, {0.0f, 0.0f, -1.0f});
    return {
        forward.x * distance,
        forward.y * distance,
        forward.z * distance,
    };
}

float cameraDepthForWorldPoint(
    const CameraView& view,
    const Vec3 point,
    const float minimumCameraDistance
) noexcept
{
    const float yaw = degreesToRadians(view.yawDegrees);
    const float pitch = degreesToRadians(view.pitchDegrees);
    const float cosYaw = std::cos(yaw);
    const float sinYaw = std::sin(yaw);
    const float cosPitch = std::cos(pitch);
    const float sinPitch = std::sin(pitch);

    const Vec3 translated{
        point.x - view.pan.x,
        point.y - view.pan.y,
        point.z - view.pan.z,
    };
    const Vec3 afterYaw{
        translated.x * cosYaw + translated.z * sinYaw,
        translated.y,
        -translated.x * sinYaw + translated.z * cosYaw,
    };
    const float cameraSpaceZ =
        afterYaw.y * sinPitch + afterYaw.z * cosPitch -
        positiveCameraDistance(view.distance, minimumCameraDistance);
    return cameraSpaceZ < -kMinimumDepth ? -cameraSpaceZ : kMinimumDepth;
}

float outlineExpansionForDepth(
    const float cameraDepth,
    const int viewportHeight,
    const float fovDegrees,
    const float outlinePixels,
    const float multiplier
) noexcept
{
    if (viewportHeight <= 0) {
        return 0.0f;
    }

    const float fovRadians = degreesToRadians(fovDegrees);
    const float worldHeight = 2.0f * cameraDepth * std::tan(fovRadians * 0.5f);
    return worldHeight * outlinePixels * multiplier / static_cast<float>(viewportHeight);
}

} // namespace ovtr::win32
