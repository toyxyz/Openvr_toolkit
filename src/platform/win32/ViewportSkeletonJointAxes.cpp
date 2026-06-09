#include "platform/win32/ViewportSkeletonJointAxes.h"

#include "math/PoseTransform.h"
#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportGlStateScope.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <gl/GL.h>

namespace ovtr::win32 {
namespace {

constexpr float kAxisLength = 0.075f;

Vec3 add(const Vec3 a, const Vec3 b) noexcept { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 sub(const Vec3 a, const Vec3 b) noexcept { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 scale(const Vec3 value, const float amount) noexcept { return {value.x * amount, value.y * amount, value.z * amount}; }

float dot(const Vec3 a, const Vec3 b) noexcept
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
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

Vec3 rotatedAxis(const MappingVirtualTarget& target, const std::array<float, 3>& axis) noexcept
{
    const std::array<float, 3> rotated = ovtr::rotatePositionByQuaternion(target.transform.rotation, axis);
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

int firstChildIndex(const ProfileSkeletonJoints& joints, const int parentIndex) noexcept
{
    for (std::size_t index = 0; index < joints.size(); ++index) {
        if (joints[index].parentIndex == parentIndex) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

Vec3 jointMainAxis(const ProfileSkeletonJoints& joints, const int jointIndex) noexcept
{
    const int child = firstChildIndex(joints, jointIndex);
    if (child >= 0) {
        return normalizeVec3(sub(joints[static_cast<std::size_t>(child)].positionMeters, joints[static_cast<std::size_t>(jointIndex)].positionMeters));
    }
    const int parent = joints[static_cast<std::size_t>(jointIndex)].parentIndex;
    if (parent >= 0) {
        return normalizeVec3(sub(joints[static_cast<std::size_t>(jointIndex)].positionMeters, joints[static_cast<std::size_t>(parent)].positionMeters));
    }
    return Vec3{0.0f, 1.0f, 0.0f};
}

Vec3 sideFromTarget(const std::array<MappingVirtualTarget, kMappingSlotCount>* targets, const MappingTrackerRole role) noexcept
{
    const MappingVirtualTarget* target = targetForRole(targets, role);
    return target != nullptr ? rotatedAxis(*target, {1.0f, 0.0f, 0.0f}) : Vec3{};
}

Vec3 forwardFromTarget(const std::array<MappingVirtualTarget, kMappingSlotCount>* targets, const MappingTrackerRole role) noexcept
{
    const MappingVirtualTarget* target = targetForRole(targets, role);
    return target != nullptr ? rotatedAxis(*target, {0.0f, 0.0f, 1.0f}) : Vec3{};
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

Vec3 segmentSideHint(
    const ProfileSkeletonJoints& joints,
    const std::array<MappingVirtualTarget, kMappingSlotCount>* targets,
    const int childIndex
) noexcept {
    const Vec3 storedHint = joints[static_cast<std::size_t>(childIndex)].sideHint;
    const MappingVirtualTarget* chest = targetForRole(targets, MappingTrackerRole::Chest);
    const MappingVirtualTarget* pelvis = targetForRole(targets, MappingTrackerRole::Pelvis);
    if (childIndex == kProfileJointSpine1 && chest != nullptr && pelvis != nullptr) {
        return add(sideFromTarget(targets, MappingTrackerRole::Chest), sideFromTarget(targets, MappingTrackerRole::Pelvis));
    }
    switch (childIndex) {
    case kProfileJointSpine:
        return sideFromTarget(targets, MappingTrackerRole::Pelvis);
    case kProfileJointSpine2:
    case kProfileJointLeftShoulder:
    case kProfileJointRightShoulder:
        return sideFromTarget(targets, MappingTrackerRole::Chest);
    case kProfileJointNeck:
    case kProfileJointHead:
    case kProfileJointHeadTopEnd:
        return sideFromTarget(targets, MappingTrackerRole::Head);
    case kProfileJointLeftUpLeg:
    case kProfileJointRightUpLeg:
        return sideFromTarget(targets, MappingTrackerRole::Pelvis);
    case kProfileJointLeftArm:
    case kProfileJointLeftForeArm:
        return storedHint;
    case kProfileJointRightArm:
    case kProfileJointRightForeArm:
        return storedHint;
    case kProfileJointLeftHand:
        return forwardFromTarget(targets, MappingTrackerRole::LeftHand);
    case kProfileJointRightHand:
        return forwardFromTarget(targets, MappingTrackerRole::RightHand);
    case kProfileJointLeftLeg:
    case kProfileJointLeftFoot:
        return limbPlaneSideHint(joints, kProfileJointLeftUpLeg, kProfileJointLeftLeg, kProfileJointLeftFoot);
    case kProfileJointRightLeg:
    case kProfileJointRightFoot:
        return limbPlaneSideHint(joints, kProfileJointRightUpLeg, kProfileJointRightLeg, kProfileJointRightFoot);
    case kProfileJointLeftToeBase:
        return sideFromTarget(targets, MappingTrackerRole::LeftFoot);
    case kProfileJointRightToeBase:
        return sideFromTarget(targets, MappingTrackerRole::RightFoot);
    default:
        return storedHint;
    }
}

int poseAxisBasisIndex(const int jointIndex, const int segmentIndex) noexcept
{
    if (jointIndex == kProfileJointLeftLeg && segmentIndex == kProfileJointLeftFoot) {
        return kProfileJointLeftLeg;
    }
    if (jointIndex == kProfileJointRightLeg && segmentIndex == kProfileJointRightFoot) {
        return kProfileJointRightLeg;
    }
    return segmentIndex;
}

void drawAxisLine(const Vec3 origin, const Vec3 axis, const RgbColor color)
{
    const Vec3 end = add(origin, scale(axis, kAxisLength));
    setGlColor(color);
    glVertex3f(origin.x, origin.y, origin.z);
    glVertex3f(end.x, end.y, end.z);
}

void drawJointAxis(const ProfileSkeletonJoints& joints, const int index, const Vec3 offset, const std::array<MappingVirtualTarget, kMappingSlotCount>* targets)
{
    const Vec3 origin = add(joints[static_cast<std::size_t>(index)].positionMeters, offset);
    const int child = firstChildIndex(joints, index);
    const int segmentIndex = child >= 0 ? child : index;
    const Vec3 yAxis = jointMainAxis(joints, index);
    Vec3 xAxis = segmentSideHint(joints, targets, segmentIndex);
    xAxis = normalizeVec3(sub(xAxis, scale(yAxis, dot(xAxis, yAxis))));
    if (!hasVector(xAxis)) {
        xAxis = normalizeVec3(cross(yAxis, std::abs(yAxis.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{0.0f, 0.0f, 1.0f}));
    }
    const Vec3 zAxis = normalizeVec3(cross(xAxis, yAxis));
    drawAxisLine(origin, xAxis, RgbColor{255, 64, 64});
    drawAxisLine(origin, yAxis, RgbColor{80, 255, 90});
    drawAxisLine(origin, zAxis, RgbColor{90, 130, 255});
}

void drawPoseJointAxis(
    const ProfileSkeletonJoints& joints,
    const std::array<Vec3, kProfileSkeletonJointCount>& sideAxes,
    const std::array<Vec3, kProfileSkeletonJointCount>& forwardAxes,
    const int index,
    const Vec3 offset
) {
    const Vec3 origin = add(joints[static_cast<std::size_t>(index)].positionMeters, offset);
    const int child = firstChildIndex(joints, index);
    const int segmentIndex = child >= 0 ? child : index;
    const Vec3 yAxis = jointMainAxis(joints, index);
    const int basisIndex = poseAxisBasisIndex(index, segmentIndex);
    Vec3 xAxis = sideAxes[static_cast<std::size_t>(basisIndex)];
    xAxis = normalizeVec3(sub(xAxis, scale(yAxis, dot(xAxis, yAxis))));
    if (!hasVector(xAxis)) {
        xAxis = normalizeVec3(cross(yAxis, forwardAxes[static_cast<std::size_t>(basisIndex)]));
    }
    const Vec3 zAxis = normalizeVec3(cross(xAxis, yAxis));
    drawAxisLine(origin, xAxis, RgbColor{255, 64, 64});
    drawAxisLine(origin, yAxis, RgbColor{80, 255, 90});
    drawAxisLine(origin, zAxis, RgbColor{90, 130, 255});
}

} // namespace

void drawSkeletonJointAxes3D(
    const ProfileSkeletonJoints& joints,
    const Vec3 offset,
    const std::array<MappingVirtualTarget, kMappingSlotCount>* targets
) {
    ScopedGlCapability lighting(GL_LIGHTING, false);
    ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
    ScopedGlLineWidth lineWidth(2.0f);
    glBegin(GL_LINES);
    for (int index = 0; index < kProfileSkeletonJointCount; ++index) {
        drawJointAxis(joints, index, offset, targets);
    }
    glEnd();
}

void drawSkeletonPoseJointAxes3D(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& joints,
    const SkeletonPose& pose,
    const Vec3 offset
) {
    const auto sideAxes = computeSkeletonPoseWorldSideAxes(rest, pose);
    const auto forwardAxes = computeSkeletonPoseWorldForwardAxes(rest, pose);
    ScopedGlCapability lighting(GL_LIGHTING, false);
    ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
    ScopedGlLineWidth lineWidth(2.0f);
    glBegin(GL_LINES);
    for (int index = 0; index < kProfileSkeletonJointCount; ++index) {
        drawPoseJointAxis(joints, sideAxes, forwardAxes, index, offset);
    }
    glEnd();
}

} // namespace ovtr::win32
