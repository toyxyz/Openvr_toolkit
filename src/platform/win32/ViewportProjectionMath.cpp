#include "platform/win32/ViewportMath.h"

#include "platform/win32/ViewportMathInternal.h"

#include <cmath>

namespace ovtr::win32 {

void perspectiveMatrix(
    const float fovyDegrees,
    const float aspect,
    const float nearPlane,
    const float farPlane,
    float* out
) noexcept
{
    if (!out) {
        return;
    }

    const float radians = degreesToRadians(fovyDegrees);
    const float f = 1.0f / std::tan(radians * 0.5f);

    for (int i = 0; i < 16; ++i) {
        out[i] = 0.0f;
    }

    out[0] = f / aspect;
    out[5] = f;
    out[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    out[11] = -1.0f;
    out[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
}

} // namespace ovtr::win32
