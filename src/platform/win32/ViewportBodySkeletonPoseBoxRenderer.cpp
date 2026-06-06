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

Vec3 boxBasisAxis(
    const ProfileSkeletonJoints& joints,
    const std::array<Vec3, kProfileSkeletonJointCount>& sideAxes,
    const std::size_t index
) noexcept {
    if (index == kProfileJointLeftUpLeg || index == kProfileJointRightUpLeg) {
        return sideAxes[static_cast<std::size_t>(kProfileJointHips)];
    }
    const int handRoot = handRootForFingerBone(index);
    if (handRoot >= 0) {
        return fingerSpreadBasisAxis(joints, sideAxes, index);
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
    for (std::size_t index = 0; index < joints.size(); ++index) {
        const ProfileSkeletonJoint& joint = joints[index];
        if (joint.parentIndex < 0 || index == kProfileJointHead) { continue; }
        const int drawParent = index == kProfileJointHeadTopEnd ? kProfileJointNeck : joint.parentIndex;
        const Vec3 parent = translated(joints[static_cast<std::size_t>(drawParent)].positionMeters, offset);
        const Vec3 child = translated(joint.positionMeters, offset);
        const BodyBoneBoxStyle style = styleForBodyBone(static_cast<int>(index), heightMeters);
        drawBeveledSegmentBoxWithBasis3D(
            parent,
            child,
            boxBasisAxis(joints, sideAxes, index),
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
