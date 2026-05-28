#include "platform/win32/ViewportSkeletalBoxRenderer.h"

#include "data/SkeletalSyntheticPose.h"
#include "math/PoseTransform.h"
#include "platform/win32/AppOriginState.h"
#include "platform/win32/AppViewportState.h"
#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportGlMatrixScope.h"
#include "platform/win32/ViewportGlStateScope.h"
#include "platform/win32/ViewportGlTextureBindingScope.h"
#include "platform/win32/ViewportRenderModelMatcap.h"

#include <array>
#include <gl/GL.h>

namespace ovtr::win32 {
namespace {

constexpr std::size_t kSkeletalPoseSlotCount = ovtr::kSkeletalHandBoneCount * 2u;
constexpr GLfloat kSkeletalBoneLineWidth = 12.0f;

struct SkeletalPoseCache {
    std::array<ovtr::PoseSample, kSkeletalPoseSlotCount> poses{};
    std::array<bool, kSkeletalPoseSlotCount> valid{};
};

std::size_t skeletalPoseSlot(const ovtr::SkeletalHandSide side, const std::uint32_t boneIndex) noexcept
{
    return (side == ovtr::SkeletalHandSide::Left ? 0u : ovtr::kSkeletalHandBoneCount) + boneIndex;
}

void drawBoxFaces(const float halfX, const float halfY, const float halfZ)
{
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-halfX, -halfY, halfZ);
    glVertex3f(halfX, -halfY, halfZ);
    glVertex3f(halfX, halfY, halfZ);
    glVertex3f(-halfX, halfY, halfZ);

    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-halfX, -halfY, -halfZ);
    glVertex3f(-halfX, halfY, -halfZ);
    glVertex3f(halfX, halfY, -halfZ);
    glVertex3f(halfX, -halfY, -halfZ);

    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-halfX, halfY, -halfZ);
    glVertex3f(-halfX, halfY, halfZ);
    glVertex3f(halfX, halfY, halfZ);
    glVertex3f(halfX, halfY, -halfZ);

    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-halfX, -halfY, -halfZ);
    glVertex3f(halfX, -halfY, -halfZ);
    glVertex3f(halfX, -halfY, halfZ);
    glVertex3f(-halfX, -halfY, halfZ);

    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(halfX, -halfY, -halfZ);
    glVertex3f(halfX, halfY, -halfZ);
    glVertex3f(halfX, halfY, halfZ);
    glVertex3f(halfX, -halfY, halfZ);

    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-halfX, -halfY, -halfZ);
    glVertex3f(-halfX, -halfY, halfZ);
    glVertex3f(-halfX, halfY, halfZ);
    glVertex3f(-halfX, halfY, -halfZ);
    glEnd();
}

ovtr::PoseSample displayPoseForState(const AppOriginState& state, ovtr::PoseSample pose)
{
    return ovtr::applyOriginToPose(
        pose,
        state.originEnabled,
        state.originOffset,
        state.originRotationDegrees
    );
}

void drawSkeletalBox(const ovtr::PoseSample& pose, AppViewportState& state)
{
    const float halfEdge = ovtr::kSkeletalBoneBoxEdgeMeters * 0.5f;
    ScopedGlMatrixStack modelView(GL_MODELVIEW);
    glTranslatef(pose.position[0], pose.position[1], pose.position[2]);
    multiplyOpenGLMatrixFromQuaternion(pose.rotation);

    ScopedGlCapability cullFace(GL_CULL_FACE, false);
    if (ensureRenderModelMatcapTexture(state)) {
        ScopedGlCapability lighting(GL_LIGHTING, false);
        ScopedGlCapability texture2D(GL_TEXTURE_2D, true);
        ScopedGlTexture2DBinding textureBinding(state.renderModelMatcapTexture.get());
        ScopedRenderModelMatcapMapping matcapMapping;
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        setGlColor(state.viewportSettings.fingerBoxColor);
        drawBoxFaces(halfEdge, halfEdge, halfEdge);
    } else {
        ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
        setGlColor(state.viewportSettings.fingerBoxColor);
        drawBoxFaces(halfEdge, halfEdge, halfEdge);
    }
}

void drawSkeletalLine(const ovtr::PoseSample& parent, const ovtr::PoseSample& child)
{
    glVertex3f(parent.position[0], parent.position[1], parent.position[2]);
    glVertex3f(child.position[0], child.position[1], child.position[2]);
}

void drawSkeletalBoneLines(const SkeletalPoseCache& cache, const AppViewportState& state)
{
    static constexpr std::array<ovtr::SkeletalHandSide, 2> kSides = {
        ovtr::SkeletalHandSide::Left,
        ovtr::SkeletalHandSide::Right,
    };

    ScopedGlCapability lighting(GL_LIGHTING, false);
    ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
    setGlColor(state.viewportSettings.fingerBoxColor);

    GLfloat previousWidth = 1.0f;
    glGetFloatv(GL_LINE_WIDTH, &previousWidth);
    glLineWidth(kSkeletalBoneLineWidth);
    glBegin(GL_LINES);
    for (const ovtr::SkeletalHandSide side : kSides) {
        for (std::uint32_t boneIndex = 0; boneIndex < ovtr::kSkeletalHandBoneCount; ++boneIndex) {
            std::uint32_t parentIndex = 0;
            if (!ovtr::skeletalBoneParentIndex(boneIndex, parentIndex)) {
                continue;
            }

            const std::size_t childSlot = skeletalPoseSlot(side, boneIndex);
            const std::size_t parentSlot = skeletalPoseSlot(side, parentIndex);
            if (cache.valid[childSlot] && cache.valid[parentSlot]) {
                drawSkeletalLine(cache.poses[parentSlot], cache.poses[childSlot]);
            }
        }
    }
    glEnd();
    glLineWidth(previousWidth);
}

} // namespace

void drawSkeletalFingerBoxes3D(
    const ovtr::PosePollResult& poses,
    const AppOriginState& originState,
    AppViewportState& viewportState
)
{
    SkeletalPoseCache cache;
    for (const ovtr::PoseSample& pose : poses.poses) {
        if (!ovtr::isSkeletalBoneRuntimeIndex(pose.runtimeIndex) ||
            (pose.flags & ovtr::PoseFlagPoseValid) == 0) {
            continue;
        }
        ovtr::SkeletalHandSide side = ovtr::SkeletalHandSide::Left;
        std::uint32_t boneIndex = 0;
        if (!ovtr::decodeSkeletalBoneRuntimeIndex(pose.runtimeIndex, side, boneIndex)) {
            continue;
        }
        const std::size_t slot = skeletalPoseSlot(side, boneIndex);
        cache.poses[slot] = displayPoseForState(originState, pose);
        cache.valid[slot] = true;
    }

    drawSkeletalBoneLines(cache, viewportState);
    for (std::size_t slot = 0; slot < cache.poses.size(); ++slot) {
        if (cache.valid[slot]) {
            drawSkeletalBox(cache.poses[slot], viewportState);
        }
    }
}

} // namespace ovtr::win32
