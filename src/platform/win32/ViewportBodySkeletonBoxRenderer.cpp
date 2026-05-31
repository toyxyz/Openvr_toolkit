#include "platform/win32/ViewportBodySkeletonBoxRenderer.h"

#include "math/PoseTransform.h"
#include "platform/win32/AppViewportState.h"
#include "platform/win32/ViewportBeveledBoxPrimitive.h"
#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportGlStateScope.h"
#include "platform/win32/ViewportGlTextureBindingScope.h"
#include "platform/win32/ViewportRenderModelMatcap.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <gl/GL.h>

namespace ovtr::win32 {
namespace {

struct BodyBoneBoxStyle {
    float halfSide = 0.02f;
    float halfDepth = 0.02f;
};

Vec3 add(const Vec3 a, const Vec3 b) noexcept
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 sub(const Vec3 a, const Vec3 b) noexcept
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 translated(const Vec3 value, const Vec3 offset) noexcept
{
    return add(value, offset);
}

Vec3 cross(const Vec3 a, const Vec3 b) noexcept
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

bool hasVector(const Vec3 value) noexcept
{
    return value.x != 0.0f || value.y != 0.0f || value.z != 0.0f;
}

Vec3 rotatedSide(const MappingVirtualTarget& target) noexcept
{
    const std::array<float, 3> rotated =
        ovtr::rotatePositionByQuaternion(target.transform.rotation, {1.0f, 0.0f, 0.0f});
    return Vec3{rotated[0], rotated[1], rotated[2]};
}

Vec3 rotatedForward(const MappingVirtualTarget& target) noexcept
{
    const std::array<float, 3> rotated =
        ovtr::rotatePositionByQuaternion(target.transform.rotation, {0.0f, 0.0f, 1.0f});
    return Vec3{rotated[0], rotated[1], rotated[2]};
}

const MappingVirtualTarget* targetForRole(
    const std::array<MappingVirtualTarget, kMappingSlotCount>* targets,
    const MappingTrackerRole role
) noexcept {
    if (targets == nullptr) {
        return nullptr;
    }
    const MappingVirtualTarget& target = (*targets)[static_cast<std::size_t>(mappingSlotForRole(role))];
    return target.valid ? &target : nullptr;
}

Vec3 targetSide(
    const std::array<MappingVirtualTarget, kMappingSlotCount>* targets,
    const MappingTrackerRole role
) noexcept {
    const MappingVirtualTarget* target = targetForRole(targets, role);
    return target != nullptr ? rotatedSide(*target) : Vec3{};
}

Vec3 targetForward(
    const std::array<MappingVirtualTarget, kMappingSlotCount>* targets,
    const MappingTrackerRole role
) noexcept {
    const MappingVirtualTarget* target = targetForRole(targets, role);
    return target != nullptr ? rotatedForward(*target) : Vec3{};
}

Vec3 limbPlaneSideHint(
    const ProfileSkeletonJoints& joints,
    const int rootIndex,
    const int midIndex,
    const int endIndex
) noexcept {
    const Vec3 root = joints[static_cast<std::size_t>(rootIndex)].positionMeters;
    const Vec3 mid = joints[static_cast<std::size_t>(midIndex)].positionMeters;
    const Vec3 end = joints[static_cast<std::size_t>(endIndex)].positionMeters;
    return normalizeVec3(cross(sub(mid, root), sub(end, mid)));
}

Vec3 boneSideHint(
    const ProfileSkeletonJoints& joints,
    const std::array<MappingVirtualTarget, kMappingSlotCount>* targets,
    const int childIndex
) noexcept {
    const Vec3 storedHint = joints[static_cast<std::size_t>(childIndex)].sideHint;
    if (targets == nullptr) {
        return storedHint;
    }
    const MappingVirtualTarget& chest = (*targets)[static_cast<std::size_t>(mappingSlotForRole(MappingTrackerRole::Chest))];
    const MappingVirtualTarget& pelvis = (*targets)[static_cast<std::size_t>(mappingSlotForRole(MappingTrackerRole::Pelvis))];
    if (childIndex == kProfileJointSpine1 && chest.valid && pelvis.valid) {
        return add(rotatedSide(chest), rotatedSide(pelvis));
    }
    switch (childIndex) {
    case kProfileJointSpine:
        return targetSide(targets, MappingTrackerRole::Pelvis);
    case kProfileJointSpine2:
    case kProfileJointLeftShoulder:
    case kProfileJointRightShoulder:
        return targetSide(targets, MappingTrackerRole::Chest);
    case kProfileJointNeck:
    case kProfileJointHeadTopEnd:
        return targetSide(targets, MappingTrackerRole::Head);
    case kProfileJointLeftUpLeg:
        return targetSide(targets, MappingTrackerRole::Pelvis);
    case kProfileJointRightUpLeg:
        return targetSide(targets, MappingTrackerRole::Pelvis);
    case kProfileJointLeftArm:
    case kProfileJointLeftForeArm:
        return limbPlaneSideHint(joints, kProfileJointLeftShoulder, kProfileJointLeftArm, kProfileJointLeftForeArm);
    case kProfileJointRightArm:
    case kProfileJointRightForeArm:
        return limbPlaneSideHint(joints, kProfileJointRightShoulder, kProfileJointRightArm, kProfileJointRightForeArm);
    case kProfileJointLeftHand:
        return targetForward(targets, MappingTrackerRole::LeftHand);
    case kProfileJointRightHand:
        return targetForward(targets, MappingTrackerRole::RightHand);
    case kProfileJointLeftLeg:
    case kProfileJointLeftFoot:
        return limbPlaneSideHint(joints, kProfileJointLeftUpLeg, kProfileJointLeftLeg, kProfileJointLeftFoot);
    case kProfileJointRightLeg:
    case kProfileJointRightFoot:
        return limbPlaneSideHint(joints, kProfileJointRightUpLeg, kProfileJointRightLeg, kProfileJointRightFoot);
    case kProfileJointLeftToeBase:
        return targetSide(targets, MappingTrackerRole::LeftFoot);
    case kProfileJointRightToeBase:
        return targetSide(targets, MappingTrackerRole::RightFoot);
    default:
        break;
    }
    return storedHint;
}

BodyBoneBoxStyle scaledStyle(
    const float heightMeters,
    const float sideMultiplier,
    const float depthMultiplier,
    const float minHalf,
    const float maxHalf
) noexcept
{
    return {
        std::clamp(heightMeters * sideMultiplier, minHalf, maxHalf),
        std::clamp(heightMeters * depthMultiplier, minHalf, maxHalf)
    };
}

BodyBoneBoxStyle styleForBone(const int childIndex, const float heightMeters) noexcept
{
    if (isProfileFingerJoint(childIndex)) {
        return scaledStyle(heightMeters, 0.005f, 0.004f, 0.004f, 0.012f);
    }
    switch (childIndex) {
    case kProfileJointSpine:
    case kProfileJointSpine1:
        return scaledStyle(heightMeters, 0.030f, 0.022f, 0.030f, 0.060f);
    case kProfileJointSpine2:
        return scaledStyle(heightMeters, 0.052f, 0.038f, 0.060f, 0.100f);
    case kProfileJointNeck:
        return scaledStyle(heightMeters, 0.010f, 0.010f, 0.014f, 0.024f);
    case kProfileJointHead:
    case kProfileJointHeadTopEnd:
        return scaledStyle(heightMeters, 0.034f, 0.034f, 0.045f, 0.070f);
    case kProfileJointLeftShoulder:
    case kProfileJointRightShoulder:
        return scaledStyle(heightMeters, 0.010f, 0.008f, 0.012f, 0.022f);
    case kProfileJointLeftArm:
    case kProfileJointRightArm:
        return scaledStyle(heightMeters, 0.017f, 0.014f, 0.020f, 0.034f);
    case kProfileJointLeftForeArm:
    case kProfileJointRightForeArm:
        return scaledStyle(heightMeters, 0.014f, 0.012f, 0.018f, 0.030f);
    case kProfileJointLeftHand:
    case kProfileJointRightHand:
        return scaledStyle(heightMeters, 0.018f, 0.006f, 0.010f, 0.035f);
    case kProfileJointLeftUpLeg:
    case kProfileJointRightUpLeg:
    case kProfileJointLeftLeg:
    case kProfileJointRightLeg:
        return scaledStyle(heightMeters, 0.018f, 0.015f, 0.022f, 0.040f);
    case kProfileJointLeftFoot:
    case kProfileJointRightFoot:
        return scaledStyle(heightMeters, 0.014f, 0.012f, 0.018f, 0.032f);
    case kProfileJointLeftToeBase:
    case kProfileJointRightToeBase:
        return scaledStyle(heightMeters, 0.024f, 0.007f, 0.012f, 0.045f);
    default:
        return scaledStyle(heightMeters, 0.016f, 0.014f, 0.018f, 0.034f);
    }
}

void drawSkeletonBoxGeometry(
    const ProfileSkeletonJoints& joints,
    const float heightMeters,
    const Vec3 offset,
    const std::array<MappingVirtualTarget, kMappingSlotCount>* targets
)
{
    for (std::size_t index = 0; index < joints.size(); ++index) {
        const ProfileSkeletonJoint& joint = joints[index];
        if (joint.parentIndex < 0) {
            continue;
        }
        if (index == kProfileJointHead) {
            continue;
        }
        const Vec3 parent = translated(joints[static_cast<std::size_t>(joint.parentIndex)].positionMeters, offset);
        const Vec3 child = translated(joint.positionMeters, offset);
        const BodyBoneBoxStyle style = styleForBone(static_cast<int>(index), heightMeters);
        if (index == kProfileJointHeadTopEnd) {
            const Vec3 neck = translated(joints[static_cast<std::size_t>(kProfileJointNeck)].positionMeters, offset);
            const Vec3 sideHint = boneSideHint(joints, targets, static_cast<int>(index));
            if (hasVector(sideHint)) {
                drawBeveledSegmentBoxWithSideHint3D(neck, child, sideHint, style.halfSide, style.halfDepth);
            } else {
                drawBeveledSegmentBox3D(neck, child, style.halfSide, style.halfDepth);
            }
        } else {
            const Vec3 sideHint = boneSideHint(joints, targets, static_cast<int>(index));
            if (hasVector(sideHint)) {
                drawBeveledSegmentBoxWithSideHint3D(parent, child, sideHint, style.halfSide, style.halfDepth);
            } else {
                drawBeveledSegmentBox3D(parent, child, style.halfSide, style.halfDepth);
            }
        }
    }
}

} // namespace

void drawBodySkeletonBoxes3D(
    AppViewportState& viewportState,
    const ProfileSkeletonJoints& joints,
    const float heightMeters,
    const Vec3 offset,
    const std::array<MappingVirtualTarget, kMappingSlotCount>* targets
)
{
    ScopedGlCapability cullFace(GL_CULL_FACE, false);
    setGlColor(viewportState.viewportSettings.bodyColor);
    if (ensureRenderModelMatcapTexture(viewportState)) {
        ScopedGlCapability lighting(GL_LIGHTING, false);
        ScopedGlCapability texture2D(GL_TEXTURE_2D, true);
        ScopedGlTexture2DBinding textureBinding(viewportState.renderModelMatcapTexture.get());
        ScopedRenderModelMatcapMapping matcapMapping;
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        drawSkeletonBoxGeometry(joints, heightMeters, offset, targets);
        return;
    }

    ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
    drawSkeletonBoxGeometry(joints, heightMeters, offset, targets);
}

} // namespace ovtr::win32
