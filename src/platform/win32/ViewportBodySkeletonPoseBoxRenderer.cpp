#include "platform/win32/ViewportBodySkeletonBoxRenderer.h"

#include "platform/win32/AppViewportState.h"
#include "platform/win32/MappingTransformMath.h"
#include "platform/win32/ViewportBeveledBoxPrimitive.h"
#include "platform/win32/ViewportBodySkeletonBoxStyle.h"
#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportGlStateScope.h"
#include "platform/win32/ViewportGlTextureBindingScope.h"
#include "platform/win32/ViewportRenderModelMatcap.h"

#include <cstddef>
#include <gl/GL.h>

namespace ovtr::win32 {
namespace {

constexpr float kAxisEpsilon = 0.0001f;
constexpr float kOppositeAxisDot = -0.999f;

Vec3 add(const Vec3 a, const Vec3 b) noexcept
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 translated(const Vec3 value, const Vec3 offset) noexcept
{
    return add(value, offset);
}

int handRootForFingerBone(const std::size_t index) noexcept
{
    if (index >= kProfileJointLeftHandThumb1 && index <= kProfileJointLeftHandPinky4) {
        return kProfileJointLeftHand;
    }
    if (index >= kProfileJointRightHandThumb1 && index <= kProfileJointRightHandPinky4) {
        return kProfileJointRightHand;
    }
    return -1;
}

ProfileSkeletonHandSide handSideForFingerBone(const std::size_t index) noexcept
{
    return index >= kProfileJointRightHandThumb1 ? ProfileSkeletonHandSide::Right : ProfileSkeletonHandSide::Left;
}

Vec3 crossVec(const Vec3 a, const Vec3 b) noexcept
{
    return Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Vec3 projectOffAxis(const Vec3 value, const Vec3 axis) noexcept
{
    return subMappingVec3(value, scaleMappingVec3(axis, dotMappingVec3(value, axis)));
}

Vec3 projectedAxisOr(const Vec3 value, const Vec3 axis, const Vec3 fallback) noexcept
{
    const Vec3 projected = projectOffAxis(value, axis);
    const Vec3 projectedFallback = projectOffAxis(fallback, axis);
    return normalizeMappingVec3Or(projected, normalizeMappingVec3Or(projectedFallback, fallback));
}

bool isThumbDistalBox(const std::size_t index) noexcept
{
    return index == static_cast<std::size_t>(kProfileJointLeftHandThumb4) ||
        index == static_cast<std::size_t>(kProfileJointRightHandThumb4);
}

Vec3 fingerSpreadBasisAxis(
    const ProfileSkeletonJoints& joints,
    const std::array<Vec3, kProfileSkeletonJointCount>& sideAxes,
    const std::size_t index
) noexcept {
    const ProfileSkeletonHandSide side = handSideForFingerBone(index);
    const int hand = profileHandRootJoint(side);
    const Vec3 wrist = joints[static_cast<std::size_t>(hand)].positionMeters;
    Vec3 forward = subMappingVec3(
        joints[static_cast<std::size_t>(profileFingerJoint(side, ProfileSkeletonFinger::Middle, 1))].positionMeters,
        wrist
    );
    forward = normalizeMappingVec3Or(forward, sideAxes[static_cast<std::size_t>(hand)]);
    Vec3 spread = subMappingVec3(
        joints[static_cast<std::size_t>(profileFingerJoint(side, ProfileSkeletonFinger::Index, 1))].positionMeters,
        joints[static_cast<std::size_t>(profileFingerJoint(side, ProfileSkeletonFinger::Pinky, 1))].positionMeters
    );
    spread = subMappingVec3(spread, scaleMappingVec3(forward, dotMappingVec3(spread, forward)));
    return normalizeMappingVec3Or(spread, sideAxes[static_cast<std::size_t>(hand)]);
}

Vec3 transportedBasisAxis(
    const Vec3 fromAxis,
    const Vec3 toAxis,
    const Vec3 basis,
    const Vec3 fallback
) noexcept {
    const Vec3 rotationAxis = crossVec(fromAxis, toAxis);
    const float sinAngle = lengthMappingVec3(rotationAxis);
    const float cosAngle = dotMappingVec3(fromAxis, toAxis);
    Vec3 transported = basis;
    if (sinAngle > kAxisEpsilon && cosAngle > kOppositeAxisDot) {
        const Vec3 axis = scaleMappingVec3(rotationAxis, 1.0f / sinAngle);
        transported = add(
            add(scaleMappingVec3(basis, cosAngle), scaleMappingVec3(crossVec(axis, basis), sinAngle)),
            scaleMappingVec3(axis, dotMappingVec3(axis, basis) * (1.0f - cosAngle))
        );
    }
    return projectedAxisOr(transported, toAxis, fallback);
}

Vec3 thumbParentBasisAxis(
    const Vec3 previousAxis,
    const Vec3 spreadAxis,
    const Vec3 handSide
) noexcept {
    return projectedAxisOr(spreadAxis, previousAxis, handSide);
}

Vec3 thumbDistalBasisAxis(
    const ProfileSkeletonJoints& joints,
    const std::array<Vec3, kProfileSkeletonJointCount>& sideAxes,
    const std::size_t index,
    const Vec3 spreadAxis
) noexcept {
    const ProfileSkeletonHandSide side = handSideForFingerBone(index);
    const int hand = profileHandRootJoint(side);
    const int thumb2 = profileFingerJoint(side, ProfileSkeletonFinger::Thumb, 2);
    const int thumb3 = profileFingerJoint(side, ProfileSkeletonFinger::Thumb, 3);
    const int thumb4 = profileFingerJoint(side, ProfileSkeletonFinger::Thumb, 4);
    const Vec3 previousAxis = normalizeMappingVec3Or(subMappingVec3(
        joints[static_cast<std::size_t>(thumb3)].positionMeters,
        joints[static_cast<std::size_t>(thumb2)].positionMeters
    ), spreadAxis);
    const Vec3 segmentAxis = normalizeMappingVec3Or(subMappingVec3(
        joints[static_cast<std::size_t>(thumb4)].positionMeters,
        joints[static_cast<std::size_t>(thumb3)].positionMeters
    ), previousAxis);
    const Vec3 handSide = sideAxes[static_cast<std::size_t>(hand)];
    const Vec3 parentBasis = thumbParentBasisAxis(previousAxis, spreadAxis, handSide);
    return transportedBasisAxis(previousAxis, segmentAxis, parentBasis, spreadAxis);
}

Vec3 visualBoxChild(
    const ProfileSkeletonJoints& rest,
    const Vec3 parent,
    const Vec3 child,
    const std::size_t index
) noexcept {
    if (!isThumbDistalBox(index)) {
        return child;
    }
    const int restParent = rest[index].parentIndex;
    if (restParent < 0) {
        return child;
    }
    const Vec3 restDelta = subMappingVec3(
        rest[index].positionMeters,
        rest[static_cast<std::size_t>(restParent)].positionMeters
    );
    const Vec3 liveDelta = subMappingVec3(child, parent);
    const float restLength = lengthMappingVec3(restDelta);
    const float liveLength = lengthMappingVec3(liveDelta);
    if (restLength <= kAxisEpsilon || liveLength <= kAxisEpsilon) {
        return child;
    }
    return add(parent, scaleMappingVec3(liveDelta, restLength / liveLength));
}

Vec3 boxBasisAxis(
    const ProfileSkeletonJoints& joints,
    const std::array<Vec3, kProfileSkeletonJointCount>& sideAxes,
    const std::array<Vec3, kProfileSkeletonJointCount>& forwardAxes,
    const std::size_t index
) noexcept {
    if (index == kProfileJointLeftUpLeg) {
        return forwardAxes[static_cast<std::size_t>(kProfileJointHips)];
    }
    if (index == kProfileJointRightUpLeg) {
        return forwardAxes[static_cast<std::size_t>(kProfileJointHips)];
    }
    if (index == kProfileJointLeftFoot) {
        return sideAxes[static_cast<std::size_t>(kProfileJointLeftLeg)];
    }
    if (index == kProfileJointRightFoot) {
        return sideAxes[static_cast<std::size_t>(kProfileJointRightLeg)];
    }
    const int handRoot = handRootForFingerBone(index);
    if (handRoot >= 0) {
        const Vec3 spreadAxis = fingerSpreadBasisAxis(joints, sideAxes, index);
        return isThumbDistalBox(index)
            ? thumbDistalBasisAxis(joints, sideAxes, index, spreadAxis)
            : spreadAxis;
    }
    return sideAxes[index];
}

void drawSkeletonPoseBoxGeometry(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& joints,
    const SkeletonPose& pose,
    const float heightMeters,
    const Vec3 offset
)
{
    const auto sideAxes = computeSkeletonPoseWorldSideAxes(rest, pose);
    const auto forwardAxes = computeSkeletonPoseWorldForwardAxes(rest, pose);
    for (std::size_t index = 0; index < joints.size(); ++index) {
        const ProfileSkeletonJoint& joint = joints[index];
        if (joint.parentIndex < 0 || index == kProfileJointHead) { continue; }
        const int drawParent = index == kProfileJointHeadTopEnd ? kProfileJointNeck : joint.parentIndex;
        const Vec3 parent = translated(joints[static_cast<std::size_t>(drawParent)].positionMeters, offset);
        const Vec3 child = visualBoxChild(rest, parent, translated(joint.positionMeters, offset), index);
        const BodyBoneBoxStyle style = styleForBodyBone(static_cast<int>(index), heightMeters);
        drawBeveledSegmentBoxWithBasis3D(
            parent,
            child,
            boxBasisAxis(joints, sideAxes, forwardAxes, index),
            style.halfSide,
            style.halfDepth
        );
    }
}

} // namespace

void drawBodySkeletonBoxesFromPose3D(
    AppViewportState& viewportState,
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& joints,
    const SkeletonPose& pose,
    const float heightMeters,
    const Vec3 offset,
    const RgbColor color
)
{
    ScopedGlCapability cullFace(GL_CULL_FACE, false);
    setGlColor(color);
    if (ensureRenderModelMatcapTexture(viewportState)) {
        ScopedGlCapability lighting(GL_LIGHTING, false);
        ScopedGlCapability texture2D(GL_TEXTURE_2D, true);
        ScopedGlTexture2DBinding textureBinding(viewportState.renderModelMatcapTexture.get());
        ScopedRenderModelMatcapMapping matcapMapping;
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        drawSkeletonPoseBoxGeometry(rest, joints, pose, heightMeters, offset);
        return;
    }

    ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
    drawSkeletonPoseBoxGeometry(rest, joints, pose, heightMeters, offset);
}

} // namespace ovtr::win32
