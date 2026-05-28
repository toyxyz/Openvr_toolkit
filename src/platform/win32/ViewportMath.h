#pragma once

namespace ovtr::win32 {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct CameraView {
    float yawDegrees = 0.0f;
    float pitchDegrees = 0.0f;
    float distance = 0.0f;
    Vec3 pan{};
};

float clampFloat(float value, float minValue, float maxValue) noexcept;
float positiveCameraDistance(float distance, float minimumDistance) noexcept;
Vec3 normalizeVec3(Vec3 value) noexcept;
Vec3 rotateByInverseViewRotation(float yawDegrees, float pitchDegrees, Vec3 cameraVector) noexcept;
Vec3 screenSpacePanOffset(const CameraView& view, int dx, int dy, float minimumCameraDistance) noexcept;
Vec3 cameraDollyOffset(const CameraView& view, float distance) noexcept;
float cameraDepthForWorldPoint(const CameraView& view, Vec3 point, float minimumCameraDistance) noexcept;
float outlineExpansionForDepth(
    float cameraDepth,
    int viewportHeight,
    float fovDegrees,
    float outlinePixels,
    float multiplier
) noexcept;
void perspectiveMatrix(float fovyDegrees, float aspect, float nearPlane, float farPlane, float* out) noexcept;

} // namespace ovtr::win32
