#include "platform/win32/MappingElbowPosePrior.h"

#include "platform/win32/MappingTransformMath.h"

namespace ovtr::win32 {
namespace {

float smoothStep(const float edge0, const float edge1, const float value) noexcept
{
    float t = (value - edge0) / (edge1 - edge0);
    if (t < 0.0f) {
        t = 0.0f;
    } else if (t > 1.0f) {
        t = 1.0f;
    }
    return t * t * (3.0f - 2.0f * t);
}

Vec3 localDirection(const float outside, const float up, const float front) noexcept
{
    return normalizeMappingVec3Or(Vec3{outside, up, front}, Vec3{0.25f, -0.05f, -0.70f});
}

Vec3 sampleCanonicalPreferred(const Vec3 local) noexcept
{
    const float outside = local.x;
    const float height = local.y;
    const float forward = local.z;
    const float closeDistance = lengthMappingVec3(local);
    const float close = 1.0f - smoothStep(0.18f, 0.46f, closeDistance);
    const float crossBody = smoothStep(0.00f, 0.36f, -outside);
    const float sideReach = smoothStep(0.10f, 0.55f, outside);
    const float high = smoothStep(0.05f, 0.55f, height);
    const float low = smoothStep(0.05f, 0.55f, -height);
    const float inFront = smoothStep(0.00f, 0.55f, forward);
    const float behind = smoothStep(0.02f, 0.45f, -forward);
    return localDirection(
        0.36f + close * 0.34f + crossBody * 0.42f + high * 0.14f +
            inFront * 0.10f + behind * 0.18f - sideReach * 0.08f,
        -0.16f - high * 0.42f - low * 0.12f - close * 0.08f -
            behind * 0.12f + crossBody * 0.06f,
        -0.70f + close * 0.26f + crossBody * 0.22f + high * 0.34f +
            inFront * 0.06f + behind * 0.18f - sideReach * 0.08f
    );
}

} // namespace

Vec3 sampleElbowPosePreferredDirection(
    const MappingTransform& chest,
    const Vec3 wrist,
    const bool left
) noexcept {
    const Vec3 chestSide = rotateMappingVector(chest, Vec3{1.0f, 0.0f, 0.0f});
    const Vec3 chestUp = rotateMappingVector(chest, Vec3{0.0f, 1.0f, 0.0f});
    const Vec3 chestForward = rotateMappingVector(chest, Vec3{0.0f, 0.0f, 1.0f});
    const Vec3 outside = left ? chestSide : scaleMappingVec3(chestSide, -1.0f);
    const Vec3 delta = subMappingVec3(wrist, chest.position);
    const Vec3 canonicalLocal{
        dotMappingVec3(delta, outside),
        dotMappingVec3(delta, chestUp),
        dotMappingVec3(delta, chestForward)
    };
    const Vec3 canonicalPreferred = sampleCanonicalPreferred(canonicalLocal);
    return normalizeMappingVec3Or(
        addMappingVec3(
            addMappingVec3(scaleMappingVec3(outside, canonicalPreferred.x),
                scaleMappingVec3(chestUp, canonicalPreferred.y)),
            scaleMappingVec3(chestForward, canonicalPreferred.z)
        ),
        scaleMappingVec3(chestForward, -1.0f)
    );
}

} // namespace ovtr::win32
