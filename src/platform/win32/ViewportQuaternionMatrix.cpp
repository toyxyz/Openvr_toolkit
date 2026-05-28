#include "platform/win32/ViewportDrawPrimitives.h"

namespace ovtr::win32 {

void multiplyOpenGLMatrixFromQuaternion(const std::array<float, 4>& q)
{
    const float x = q[0];
    const float y = q[1];
    const float z = q[2];
    const float w = q[3];

    const float xx = x * x;
    const float yy = y * y;
    const float zz = z * z;
    const float xy = x * y;
    const float xz = x * z;
    const float yz = y * z;
    const float wx = w * x;
    const float wy = w * y;
    const float wz = w * z;

    const float matrix[16] = {
        1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy), 0.0f,
        2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx), 0.0f,
        2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    glMultMatrixf(matrix);
}

} // namespace ovtr::win32
