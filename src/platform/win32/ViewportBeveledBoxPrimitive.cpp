#include "platform/win32/ViewportBeveledBoxPrimitive.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <algorithm>
#include <cmath>
#include <windows.h>
#include <gl/GL.h>

namespace ovtr::win32 {
namespace {

Vec3 add(const Vec3 a, const Vec3 b) noexcept { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 sub(const Vec3 a, const Vec3 b) noexcept { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 scale(const Vec3 value, const float amount) noexcept { return {value.x * amount, value.y * amount, value.z * amount}; }

Vec3 cross(const Vec3 a, const Vec3 b) noexcept
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

float length(const Vec3 value) noexcept
{
    return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

Vec3 worldPoint(const Vec3 center, const Vec3 xAxis, const Vec3 yAxis, const Vec3 zAxis, const Vec3 local) noexcept
{
    return add(add(add(center, scale(xAxis, local.x)), scale(yAxis, local.y)), scale(zAxis, local.z));
}

void vertex(const Vec3 center, const Vec3 xAxis, const Vec3 yAxis, const Vec3 zAxis, const Vec3 local)
{
    const Vec3 value = worldPoint(center, xAxis, yAxis, zAxis, local);
    glVertex3f(value.x, value.y, value.z);
}

void normal(const Vec3 xAxis, const Vec3 yAxis, const Vec3 zAxis, const Vec3 local)
{
    const Vec3 value = normalizeVec3(add(add(scale(xAxis, local.x), scale(yAxis, local.y)), scale(zAxis, local.z)));
    glNormal3f(value.x, value.y, value.z);
}

void faceQuad(
    const Vec3 center,
    const Vec3 xAxis,
    const Vec3 yAxis,
    const Vec3 zAxis,
    const Vec3 n,
    const Vec3 a,
    const Vec3 b,
    const Vec3 c,
    const Vec3 d
)
{
    normal(xAxis, yAxis, zAxis, n);
    vertex(center, xAxis, yAxis, zAxis, a);
    vertex(center, xAxis, yAxis, zAxis, b);
    vertex(center, xAxis, yAxis, zAxis, c);
    vertex(center, xAxis, yAxis, zAxis, d);
}

void faceTri(
    const Vec3 center,
    const Vec3 xAxis,
    const Vec3 yAxis,
    const Vec3 zAxis,
    const Vec3 n,
    const Vec3 a,
    const Vec3 b,
    const Vec3 c
)
{
    normal(xAxis, yAxis, zAxis, n);
    vertex(center, xAxis, yAxis, zAxis, a);
    vertex(center, xAxis, yAxis, zAxis, b);
    vertex(center, xAxis, yAxis, zAxis, c);
}

Vec3 perpendicularBasis(const Vec3 axis) noexcept
{
    const Vec3 up = std::fabs(axis.y) < 0.92f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{0.0f, 0.0f, 1.0f};
    return normalizeVec3(cross(axis, up));
}

Vec3 projectedSideBasis(const Vec3 axis, const Vec3 sideHint) noexcept
{
    const float dot = axis.x * sideHint.x + axis.y * sideHint.y + axis.z * sideHint.z;
    const Vec3 projected = sub(sideHint, scale(axis, dot));
    return length(projected) > 0.0001f ? normalizeVec3(projected) : perpendicularBasis(axis);
}

void drawBeveledBoxWithBasis(
    const Vec3 start,
    const Vec3 end,
    const Vec3 xAxis,
    const float halfSide,
    const float halfDepth
)
{
    const Vec3 delta = sub(end, start);
    const float boxLength = length(delta);
    if (boxLength <= 0.0001f) {
        return;
    }

    const Vec3 yAxis = scale(delta, 1.0f / boxLength);
    const Vec3 zAxis = normalizeVec3(cross(yAxis, xAxis));
    const Vec3 center = scale(add(start, end), 0.5f);
    const float halfLength = boxLength * 0.5f;
    const float bevel = std::min({halfSide, halfDepth, halfLength}) * 0.28f;
    const float x = halfSide;
    const float y = halfLength;
    const float z = halfDepth;
    const float ix = std::max(0.0f, x - bevel);
    const float iy = std::max(0.0f, y - bevel);
    const float iz = std::max(0.0f, z - bevel);

    glBegin(GL_QUADS);
    for (const float s : {-1.0f, 1.0f}) {
        faceQuad(center, xAxis, yAxis, zAxis, {s, 0.0f, 0.0f}, {s * x, -iy, -iz}, {s * x, iy, -iz}, {s * x, iy, iz}, {s * x, -iy, iz});
        faceQuad(center, xAxis, yAxis, zAxis, {0.0f, s, 0.0f}, {-ix, s * y, -iz}, {-ix, s * y, iz}, {ix, s * y, iz}, {ix, s * y, -iz});
        faceQuad(center, xAxis, yAxis, zAxis, {0.0f, 0.0f, s}, {-ix, -iy, s * z}, {ix, -iy, s * z}, {ix, iy, s * z}, {-ix, iy, s * z});
    }

    for (const float sy : {-1.0f, 1.0f}) {
        for (const float sz : {-1.0f, 1.0f}) {
            faceQuad(center, xAxis, yAxis, zAxis, {0.0f, sy, sz}, {-ix, sy * y, sz * iz}, {ix, sy * y, sz * iz}, {ix, sy * iy, sz * z}, {-ix, sy * iy, sz * z});
        }
    }
    for (const float sx : {-1.0f, 1.0f}) {
        for (const float sz : {-1.0f, 1.0f}) {
            faceQuad(center, xAxis, yAxis, zAxis, {sx, 0.0f, sz}, {sx * x, -iy, sz * iz}, {sx * x, iy, sz * iz}, {sx * ix, iy, sz * z}, {sx * ix, -iy, sz * z});
        }
    }
    for (const float sx : {-1.0f, 1.0f}) {
        for (const float sy : {-1.0f, 1.0f}) {
            faceQuad(center, xAxis, yAxis, zAxis, {sx, sy, 0.0f}, {sx * x, sy * iy, -iz}, {sx * x, sy * iy, iz}, {sx * ix, sy * y, iz}, {sx * ix, sy * y, -iz});
        }
    }
    glEnd();

    glBegin(GL_TRIANGLES);
    for (const float sx : {-1.0f, 1.0f}) {
        for (const float sy : {-1.0f, 1.0f}) {
            for (const float sz : {-1.0f, 1.0f}) {
                faceTri(center, xAxis, yAxis, zAxis, {sx, sy, sz}, {sx * x, sy * iy, sz * iz}, {sx * ix, sy * y, sz * iz}, {sx * ix, sy * iy, sz * z});
            }
        }
    }
    glEnd();
}

} // namespace

void drawBeveledSegmentBox3D(const Vec3 start, const Vec3 end, const float halfSide, const float halfDepth)
{
    const Vec3 delta = sub(end, start);
    const float boxLength = length(delta);
    if (boxLength <= 0.0001f) {
        return;
    }

    const Vec3 yAxis = scale(delta, 1.0f / boxLength);
    drawBeveledBoxWithBasis(start, end, perpendicularBasis(yAxis), halfSide, halfDepth);
}

void drawBeveledSegmentBoxWithSideHint3D(
    const Vec3 start,
    const Vec3 end,
    const Vec3 sideHint,
    const float halfSide,
    const float halfDepth
) {
    const Vec3 delta = sub(end, start);
    const float boxLength = length(delta);
    if (boxLength <= 0.0001f) {
        return;
    }
    const Vec3 yAxis = scale(delta, 1.0f / boxLength);
    drawBeveledBoxWithBasis(start, end, projectedSideBasis(yAxis, sideHint), halfSide, halfDepth);
}

void drawBeveledSegmentBoxWithBasis3D(
    const Vec3 start,
    const Vec3 end,
    const Vec3 xAxis,
    const float halfSide,
    const float halfDepth
) {
    const Vec3 delta = sub(end, start);
    const float boxLength = length(delta);
    if (boxLength <= 0.0001f) {
        return;
    }
    const Vec3 yAxis = scale(delta, 1.0f / boxLength);
    drawBeveledBoxWithBasis(start, end, projectedSideBasis(yAxis, xAxis), halfSide, halfDepth);
}

} // namespace ovtr::win32
