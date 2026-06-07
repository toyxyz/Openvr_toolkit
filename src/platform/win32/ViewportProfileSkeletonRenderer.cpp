#include "platform/win32/ViewportProfileSkeletonRenderer.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/MappingCalibrationSolve.h"
#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonRecording.h"
#include "platform/win32/ViewportBodySkeletonBoxRenderer.h"
#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportGlStateScope.h"
#include "platform/win32/ViewportSkeletonJointAxes.h"
#include "platform/win32/VmcStreamingPose.h"
#include "platform/win32/Win32String.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <gl/GL.h>
#include <string>

namespace ovtr::win32 {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr int kSphereStacks = 8;
constexpr int kSphereSlices = 12;
constexpr float kActorLabelPixelLift = 18.0f;
constexpr RgbColor kTargetColor{80, 190, 255};
constexpr RgbColor kPoleColor{255, 210, 90};

Vec3 translated(const Vec3 position, const Vec3 offset) noexcept
{
    return Vec3{position.x + offset.x, position.y + offset.y, position.z + offset.z};
}

void drawBoneLines(const ProfileSkeletonJoints& joints, const Vec3 offset, const RgbColor color)
{
    ScopedGlLineWidth lineWidth(4.0f);
    setGlColor(color);
    glBegin(GL_LINES);
    for (const ProfileSkeletonJoint& joint : joints) {
        if (joint.parentIndex < 0) {
            continue;
        }
        const Vec3 parent = translated(joints[static_cast<std::size_t>(joint.parentIndex)].positionMeters, offset);
        const Vec3 child = translated(joint.positionMeters, offset);
        glVertex3f(parent.x, parent.y, parent.z);
        glVertex3f(child.x, child.y, child.z);
    }
    glEnd();
}

void sphereVertex(const Vec3 center, const float radius, const float phi, const float theta)
{
    const float cosPhi = std::cos(phi);
    const float nx = cosPhi * std::cos(theta);
    const float ny = std::sin(phi);
    const float nz = cosPhi * std::sin(theta);
    glNormal3f(nx, ny, nz);
    glVertex3f(center.x + nx * radius, center.y + ny * radius, center.z + nz * radius);
}

void drawSphere(const Vec3 center, const float radius)
{
    for (int stack = 0; stack < kSphereStacks; ++stack) {
        const float phi0 = -kPi * 0.5f + kPi * static_cast<float>(stack) / kSphereStacks;
        const float phi1 = -kPi * 0.5f + kPi * static_cast<float>(stack + 1) / kSphereStacks;
        glBegin(GL_QUAD_STRIP);
        for (int slice = 0; slice <= kSphereSlices; ++slice) {
            const float theta = 2.0f * kPi * static_cast<float>(slice) / kSphereSlices;
            sphereVertex(center, radius, phi0, theta);
            sphereVertex(center, radius, phi1, theta);
        }
        glEnd();
    }
}

void drawJointSpheres(const ProfileSkeletonJoints& joints, const float radius, const Vec3 offset, const RgbColor color)
{
    setGlColor(color);
    for (const ProfileSkeletonJoint& joint : joints) {
        drawSphere(translated(joint.positionMeters, offset), radius);
    }
}

void drawSkeletonLineJoints(
    const ProfileSkeletonJoints& joints,
    const float heightMeters,
    const Vec3 offset,
    const RgbColor color
)
{
    const float sphereRadius = std::clamp(heightMeters * 0.01f, 0.012f, 0.035f);
    drawBoneLines(joints, offset, color);
    drawJointSpheres(joints, sphereRadius, offset, color);
}

void drawSkeletonJoints(
    AppViewportState& viewportState,
    const ProfileSkeletonJoints& joints,
    const float heightMeters,
    const Vec3 offset,
    const RgbColor color,
    const std::array<MappingVirtualTarget, kMappingSlotCount>* targets = nullptr
)
{
    if (viewportState.viewportSettings.skeletonDisplayType == SkeletonDisplayType::Box) {
        drawBodySkeletonBoxes3D(viewportState, joints, heightMeters, offset, color, targets);
        return;
    }
    drawSkeletonLineJoints(joints, heightMeters, offset, color);
}

void drawSkeletonPose(
    AppViewportState& viewportState,
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& joints,
    const SkeletonPose& pose,
    const float heightMeters,
    const Vec3 offset,
    const RgbColor color
)
{
    if (viewportState.viewportSettings.skeletonDisplayType == SkeletonDisplayType::Box) {
        drawBodySkeletonBoxesFromPose3D(viewportState, rest, joints, pose, heightMeters, offset, color);
        return;
    }
    drawSkeletonLineJoints(joints, heightMeters, offset, color);
}

ProfileSkeletonJoints drawSkeletonProfile(AppViewportState& viewportState, const BodyProfile& profile, const Vec3 offset)
{
    const ProfileSkeletonJoints joints = buildProfileSkeletonJoints(profile);
    drawSkeletonJoints(
        viewportState,
        joints,
        computedProfileHeightCm(profile) * 0.01f,
        offset,
        viewportState.viewportSettings.bodyColor
    );
    return joints;
}

Vec3 actorOffset() noexcept
{
    return Vec3{0.0f, 0.0f, 0.0f};
}

void drawIkTargetJointLine(
    const MappingActor& actor,
    const Vec3 offset,
    const MappingTrackerRole targetRole,
    const int jointIndex
)
{
    const auto slot = static_cast<std::size_t>(mappingSlotForRole(targetRole));
    const MappingVirtualTarget& target = actor.liveVirtualTargets[slot];
    if (!target.valid) {
        return;
    }
    const Vec3 targetPosition = translated(target.transform.position, offset);
    const Vec3 jointPosition = translated(actor.liveJoints[static_cast<std::size_t>(jointIndex)].positionMeters, offset);
    glVertex3f(targetPosition.x, targetPosition.y, targetPosition.z);
    glVertex3f(jointPosition.x, jointPosition.y, jointPosition.z);
}

void drawMappingDebug(const MappingActor& actor, const Vec3 offset)
{
    setGlColor(kTargetColor);
    for (const MappingVirtualTarget& target : actor.liveVirtualTargets) {
        if (target.valid) {
            drawSphere(translated(target.transform.position, offset), 0.018f);
        }
    }

    ScopedGlLineWidth lineWidth(2.0f);
    setGlColor(kPoleColor);
    glBegin(GL_LINES);
    for (const MappingDebugPole& pole : actor.liveDebugPoles) {
        if (!pole.valid) {
            continue;
        }
        const Vec3 root = translated(pole.root, offset);
        const Vec3 point = translated(pole.pole, offset);
        glVertex3f(root.x, root.y, root.z);
        glVertex3f(point.x, point.y, point.z);
    }
    drawIkTargetJointLine(actor, offset, MappingTrackerRole::LeftArm, kProfileJointLeftArm);
    drawIkTargetJointLine(actor, offset, MappingTrackerRole::RightArm, kProfileJointRightArm);
    drawIkTargetJointLine(actor, offset, MappingTrackerRole::LeftLeg, kProfileJointLeftLeg);
    drawIkTargetJointLine(actor, offset, MappingTrackerRole::RightLeg, kProfileJointRightLeg);
    glEnd();
}

} // namespace

void drawProfileSkeletonPreview3D(
    const AppProfileState& profileState,
    const AppDebugUiState& debugState,
    AppViewportState& viewportState
)
{
    if (!profileState.profilePreviewEnabled) {
        return;
    }

    ScopedGlCapability lighting(GL_LIGHTING, false);
    ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
    const ProfileSkeletonJoints joints = drawSkeletonProfile(viewportState, profileState.profile, Vec3{0.0f, 0.0f, 0.0f});
    if (debugState.debugMonitorVisible) {
        drawSkeletonJointAxes3D(joints, Vec3{0.0f, 0.0f, 0.0f});
    }
}

void drawMappingActors3D(
    AppProfileState& profileState,
    const AppRuntimeState& runtimeState,
    const AppOriginState& originState,
    AppRecordingState& recordingState,
    AppStreamingState& streamingState,
    AppDebugUiState& debugState,
    AppViewportState& viewportState
)
{
    if (profileState.mappingActors.empty()) {
        return;
    }
    ScopedGlCapability lighting(GL_LIGHTING, false);
    ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
    const bool selectedCalibrated = std::any_of(
        profileState.mappingActors.begin(),
        profileState.mappingActors.end(),
        [&](const MappingActor& actor) {
            return actor.calibrated && actor.id == profileState.selectedMappingActorId;
        }
    );
    bool streamedActor = false;
    for (MappingActor& actor : profileState.mappingActors) {
        if (actor.calibrated) {
            updateCalibratedMappingActorJoints(
                actor,
                runtimeState.poses,
                originState.originEnabled,
                originState.originOffset,
                originState.originRotationDegrees
            );
            if (actor.liveJointsValid) {
                recordSkeletonFrameIfRecording(recordingState, actor, std::chrono::steady_clock::now());
                const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(actor.profile);
                const bool shouldStream = !streamedActor &&
                    (!selectedCalibrated || actor.id == profileState.selectedMappingActorId);
                if (shouldStream) {
                    std::string error;
                    if (sendMappingActorVmcPose(streamingState, actor, error)) {
                        streamingState.vmcSendErrorLogged = false;
                    } else if (!streamingState.vmcSendErrorLogged) {
                        appendDebugLog(debugState, L"VMC streaming send failed: " + widen(error));
                        streamingState.vmcSendErrorLogged = true;
                    }
                    streamedActor = true;
                }
                drawSkeletonPose(
                    viewportState,
                    rest,
                    actor.liveJoints,
                    actor.liveSkeletonPose,
                    computedProfileHeightCm(actor.profile) * 0.01f,
                    actorOffset(),
                    actor.skeletonColor
                );
                if (debugState.debugMonitorVisible) {
                    drawSkeletonPoseJointAxes3D(rest, actor.liveJoints, actor.liveSkeletonPose, actorOffset());
                }
                if (debugState.debugMonitorVisible && actor.id == profileState.selectedMappingActorId) {
                    drawMappingDebug(actor, actorOffset());
                }
                continue;
            }
        }
        const ProfileSkeletonJoints joints = buildProfileSkeletonJoints(actor.profile);
        drawSkeletonJoints(
            viewportState,
            joints,
            computedProfileHeightCm(actor.profile) * 0.01f,
            actorOffset(),
            actor.skeletonColor
        );
        if (debugState.debugMonitorVisible) {
            drawSkeletonJointAxes3D(joints, actorOffset());
        }
    }
}

void drawMappingActorLabels3D(const AppProfileState& profileState, const GLuint fontBase)
{
    if (fontBase == 0 || profileState.mappingActors.empty()) {
        return;
    }
    for (std::size_t index = 0; index < profileState.mappingActors.size(); ++index) {
        const MappingActor& actor = profileState.mappingActors[index];
        const BodyProfile& profile = actor.profile;
        const Vec3 offset = actorOffset();
        const Vec3 labelPosition = actor.calibrated && actor.liveJointsValid
            ? actor.liveJoints[kProfileJointHeadTopEnd].positionMeters
            : Vec3{0.0f, computedProfileHeightCm(profile) * 0.01f, 0.0f};
        glRasterPos3f(offset.x + labelPosition.x, offset.y + labelPosition.y, offset.z + labelPosition.z);
        offsetRasterPositionPixels(0.0f, kActorLabelPixelLift);
        drawLabelText3D(narrow(effectiveMappingActorName(actor)), fontBase);
    }
}

} // namespace ovtr::win32
