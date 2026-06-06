#ifdef _WIN32
#include "TestCases.h"
#include "TestSupport.h"
#include "platform/win32/AppProfileState.h"
#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonGltfAnimationKeys.h"
#include "platform/win32/SkeletonGltfExporter.h"
#include "platform/win32/SkeletonGltfPose.h"
#include "platform/win32/SkeletonPose.h"
#include "platform/win32/SkeletonRecording.h"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>

namespace ovtr::test {
namespace {

bool contains(const std::string& text, const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}

int countOccurrences(const std::string& text, const std::string& needle)
{
    int count = 0;
    std::size_t pos = 0;
    while ((pos = text.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.size();
    }
    return count;
}

std::uint32_t u32(const std::vector<std::uint8_t>& bytes, const std::size_t offset)
{
    return readLittleEndianUint32(bytes, offset);
}

std::string glbJson(const std::filesystem::path& path)
{
    const std::vector<std::uint8_t> bytes = readBinaryFile(path);
    require(bytes.size() > 20, "GLB too small");
    require(u32(bytes, 0) == 0x46546c67u, "GLB magic mismatch");
    const std::uint32_t jsonLength = u32(bytes, 12);
    require(u32(bytes, 16) == 0x4e4f534au, "GLB JSON chunk mismatch");
    require(bytes.size() >= 20ULL + jsonLength, "GLB JSON chunk truncated");
    return std::string(bytes.begin() + 20, bytes.begin() + 20 + jsonLength);
}

void requireVecNear(const ovtr::win32::Vec3 a, const ovtr::win32::Vec3 b, const std::string& message)
{
    require(std::abs(a.x - b.x) < 0.0001f, message + " x");
    require(std::abs(a.y - b.y) < 0.0001f, message + " y");
    require(std::abs(a.z - b.z) < 0.0001f, message + " z");
}

float dotVec(const ovtr::win32::Vec3 a, const ovtr::win32::Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

void requireDirectionNear(const ovtr::win32::Vec3 value, const ovtr::win32::Vec3 expected, const std::string& message)
{
    require(dotVec(value, expected) > 0.999f, message);
}

void requireQuatSame(
    const std::array<float, 4>& a,
    const std::array<float, 4>& b,
    const std::string& message
) {
    const float dot = std::abs(a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]);
    require(dot > 0.9999f, message);
}

} // namespace

void testWin32SkeletonPoseRestWorldJoints()
{
    using namespace ovtr::win32;

    const BodyProfile profile;
    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(profile);
    const SkeletonPose pose = makeRestSkeletonPose(rest);
    const ProfileSkeletonJoints world = computeSkeletonPoseWorldJoints(rest, pose);
    for (int index = 0; index < kProfileSkeletonJointCount; ++index) {
        requireVecNear(
            world[static_cast<std::size_t>(index)].positionMeters,
            rest[static_cast<std::size_t>(index)].positionMeters,
            "rest skeleton world joint mismatch"
        );
    }
}

void testWin32SkeletonConnectorRollStabilization()
{
    using namespace ovtr::win32;

    const BodyProfile profile;
    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(profile);
    ProfileSkeletonJoints joints = rest;
    joints[kProfileJointLeftArm].positionMeters.z += 0.20f;
    joints[kProfileJointLeftForeArm].positionMeters.z += 0.20f;
    joints[kProfileJointLeftLeg].positionMeters.z -= 0.15f;
    joints[kProfileJointLeftFoot].positionMeters.z -= 0.15f;

    SkeletonPose pose = makeSkeletonPoseFromWorldJoints(rest, joints);
    const auto beforeRotations = computeSkeletonPoseWorldRotations(rest, pose);
    const ProfileSkeletonJoints beforeJoints = computeSkeletonPoseWorldJoints(rest, pose);
    stabilizeSkeletonConnectorRolls(rest, pose);
    const auto afterRotations = computeSkeletonPoseWorldRotations(rest, pose);
    const ProfileSkeletonJoints afterJoints = computeSkeletonPoseWorldJoints(rest, pose);

    requireQuatSame(afterRotations[kProfileJointLeftShoulder], beforeRotations[kProfileJointLeftShoulder], "shoulder connector roll");
    requireQuatSame(afterRotations[kProfileJointLeftUpLeg], beforeRotations[kProfileJointLeftUpLeg], "hip connector roll");
    requireQuatSame(afterRotations[kProfileJointLeftArm], beforeRotations[kProfileJointLeftArm], "arm child rotation");
    requireQuatSame(afterRotations[kProfileJointLeftLeg], beforeRotations[kProfileJointLeftLeg], "leg child rotation");
    for (int index = 0; index < kProfileSkeletonJointCount; ++index) {
        requireVecNear(
            afterJoints[static_cast<std::size_t>(index)].positionMeters,
            beforeJoints[static_cast<std::size_t>(index)].positionMeters,
            "connector roll stabilization moved joints"
        );
    }
}

void testWin32SkeletonLowerBoneIgnoresEndEffectorRotation()
{
    using namespace ovtr::win32;

    const BodyProfile profile;
    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(profile);
    ProfileSkeletonJoints joints = rest;
    joints[kProfileJointLeftForeArm].sideHint = Vec3{0.0f, 0.0f, 1.0f};
    joints[kProfileJointLeftHand].positionMeters.y += 0.08f;
    joints[kProfileJointLeftFoot].sideHint = Vec3{1.0f, 0.0f, 0.0f};
    joints[kProfileJointLeftToeBase].positionMeters.x += 0.08f;

    const auto rotations = computeSkeletonPoseWorldRotations(rest, makeSkeletonPoseFromWorldJoints(rest, joints));
    joints[kProfileJointLeftHand].positionMeters.y -= 0.16f;
    joints[kProfileJointLeftToeBase].positionMeters.x -= 0.16f;
    const auto movedRotations = computeSkeletonPoseWorldRotations(rest, makeSkeletonPoseFromWorldJoints(rest, joints));

    requireQuatSame(rotations[kProfileJointLeftForeArm], movedRotations[kProfileJointLeftForeArm], "forearm end rotation");
    requireQuatSame(rotations[kProfileJointLeftFoot], movedRotations[kProfileJointLeftFoot], "lowerleg end rotation");
}

void testWin32SkeletonUpperArmIgnoresForearmDirection()
{
    using namespace ovtr::win32;

    const BodyProfile profile;
    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(profile);
    ProfileSkeletonJoints joints = rest;
    joints[kProfileJointLeftArm].sideHint = Vec3{0.0f, 0.0f, 1.0f};
    joints[kProfileJointLeftForeArm].positionMeters.z += 0.12f;
    const auto rotations = computeSkeletonPoseWorldRotations(rest, makeSkeletonPoseFromWorldJoints(rest, joints));
    joints[kProfileJointLeftForeArm].positionMeters.z -= 0.24f;
    const auto movedRotations = computeSkeletonPoseWorldRotations(rest, makeSkeletonPoseFromWorldJoints(rest, joints));
    requireQuatSame(rotations[kProfileJointLeftArm], movedRotations[kProfileJointLeftArm], "upper arm child direction");
}

void testWin32SkeletonWorldSideAxesUseRestBasis()
{
    using namespace ovtr::win32;

    const BodyProfile profile;
    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(profile);
    ProfileSkeletonJoints joints = rest;
    joints[kProfileJointLeftArm].sideHint = Vec3{0.0f, 0.0f, 1.0f};
    joints[kProfileJointLeftForeArm].sideHint = joints[kProfileJointLeftArm].sideHint;
    const SkeletonPose pose = makeSkeletonPoseFromWorldJoints(rest, joints);
    const auto sideAxes = computeSkeletonPoseWorldSideAxes(rest, pose);
    requireDirectionNear(sideAxes[kProfileJointLeftArm], Vec3{0.0647f, 0.0f, 0.9979f}, "upper arm side axis");
    requireDirectionNear(sideAxes[kProfileJointLeftForeArm], Vec3{-0.0771f, 0.0f, 0.9970f}, "forearm side axis");
}

void testWin32SkeletonGltfExport()
{
    using namespace ovtr::win32;

    MappingActor actor;
    actor.id = 7;
    actor.profile.name = L"gltf_actor";
    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(actor.profile);
    actor.liveJoints = rest;
    actor.liveSkeletonPose = makeRestSkeletonPose(rest);
    actor.liveJointsValid = true;
    const SkeletonPose gltfPose = makeSkeletonGltfExportPose(rest, actor.liveSkeletonPose, true);
    requireVecNear(gltfPose.bones[kProfileJointHips].localTranslationMeters, Vec3{}, "gltf root object head");
    requireVecNear(
        gltfPose.bones[kProfileJointSpine].localTranslationMeters,
        rest[kProfileJointHips].positionMeters,
        "gltf spine root motion head"
    );
    requireVecNear(gltfPose.bones[kProfileJointSpine1].localTranslationMeters, Vec3{0.0f, 0.18f, 0.0f}, "gltf spine1 head");
    requireVecNear(gltfPose.bones[kProfileJointSpine2].localTranslationMeters, Vec3{0.0f, 0.17f, 0.0f}, "gltf spine2 head");
    requireVecNear(gltfPose.bones[kProfileJointNeck].localTranslationMeters, Vec3{0.0f, 0.18f, 0.0f}, "gltf neck head");
    requireVecNear(
        gltfPose.bones[kProfileJointLeftShoulder].localTranslationMeters,
        Vec3{0.0f, 0.18f, 0.0f},
        "gltf left shoulder head"
    );
    requireVecNear(
        gltfPose.bones[kProfileJointLeftArm].localTranslationMeters,
        Vec3{0.0f, 0.205f, 0.0f},
        "gltf left arm head"
    );
    requireVecNear(
        gltfPose.bones[kProfileJointLeftForeArm].localTranslationMeters,
        Vec3{0.0f, 0.3108f, 0.0f},
        "gltf left forearm head"
    );
    requireVecNear(
        gltfPose.bones[kProfileJointLeftHand].localTranslationMeters,
        Vec3{0.0f, 0.2606f, 0.0f},
        "gltf left hand head"
    );
    requireVecNear(
        gltfPose.bones[kProfileJointLeftUpLeg].localTranslationMeters,
        Vec3{0.14f, 0.0f, 0.0f},
        "gltf left upleg head"
    );
    requireVecNear(
        gltfPose.bones[kProfileJointLeftLeg].localTranslationMeters,
        Vec3{0.0f, 0.4204f, 0.0f},
        "gltf left leg head"
    );
    requireVecNear(
        gltfPose.bones[kProfileJointLeftFoot].localTranslationMeters,
        Vec3{0.0f, 0.4004f, 0.0f},
        "gltf left foot head"
    );
    ProfileSkeletonJoints backwardJoints = rest;
    for (ProfileSkeletonJoint& joint : backwardJoints) {
        joint.positionMeters.x = -joint.positionMeters.x;
        joint.positionMeters.z = -joint.positionMeters.z;
        joint.sideHint = Vec3{-1.0f, 0.0f, 0.0f};
    }
    const SkeletonPose backward = makeSkeletonPoseFromWorldJoints(rest, backwardJoints);
    const SkeletonPose backwardGltf = makeSkeletonGltfExportPose(rest, backward);
    const SkeletonPose backwardSnapped = makeSkeletonGltfExportPose(rest, backward, true);
    const auto& freeRoot = backwardGltf.bones[kProfileJointSpine].localRotation;
    const auto& snappedRoot = backwardSnapped.bones[kProfileJointSpine].localRotation;
    const float rootDot = std::abs(
        freeRoot[0] * snappedRoot[0] + freeRoot[1] * snappedRoot[1] +
        freeRoot[2] * snappedRoot[2] + freeRoot[3] * snappedRoot[3]
    );
    require(rootDot < 0.999f, "gltf animation pose should not snap backward-facing root roll to bind pose");
    std::vector<SkeletonPose> keyedPoses(2, actor.liveSkeletonPose);
    keyedPoses[0].bones[kProfileJointLeftArm].localRotation = {0.0f, 0.0f, 0.70710677f, 0.70710677f};
    keyedPoses[1].bones[kProfileJointLeftArm].localRotation = {0.0f, 0.0f, -0.70710677f, -0.70710677f};
    const std::vector<float> rotationKeys = continuousJointRotationKeys(keyedPoses, kProfileJointLeftArm);
    require(rotationKeys[2] > 0.0f && rotationKeys[6] > 0.0f, "gltf rotation keys should keep quaternion signs continuous");

    SkeletonRecordingClip clip;
    beginSkeletonRecording(clip, actor);
    clip.frames.push_back(SkeletonRecordingFrame{0.0, actor.liveSkeletonPose});
    SkeletonPose moved = actor.liveSkeletonPose;
    moved.timeSeconds = 1.0 / 60.0;
    moved.bones[kProfileJointHips].localTranslationMeters.y += 0.10f;
    clip.frames.push_back(SkeletonRecordingFrame{moved.timeSeconds, moved});
    finishSkeletonRecording(clip);

    const std::filesystem::path outputDir = std::filesystem::current_path() / ".tmp_ovtr_skeleton_gltf";
    const std::filesystem::path outputPath = outputDir / "actor_skeleton.glb";
    std::string error;
    require(exportSkeletonRecordingToGlb(clip, outputPath, error), "Skeleton GLB export failed: " + error);

    const std::string json = glbJson(outputPath);
    require(contains(json, "\"skins\"") && contains(json, "\"name\": \"rig\""), "Skeleton GLB missing rig skin");
    require(contains(json, "\"inverseBindMatrices\""), "Skeleton GLB missing inverse bind matrices");
    require(!contains(json, "OpenVR_Skeleton_Root") && !contains(json, "\"Armature\""), "Skeleton GLB should omit containers");
    require(contains(json, "\"name\": \"root\""), "Skeleton GLB missing root");
    require(contains(json, "\"skeleton\": 1"), "Skeleton GLB skin root should be Spine");
    require(contains(json, "\"name\": \"Spine2\""), "Skeleton GLB missing Spine2");
    require(contains(json, "\"name\": \"LeftShoulder\""), "Skeleton GLB missing LeftShoulder");
    require(contains(json, "\"name\": \"LeftForeArm\""), "Skeleton GLB missing LeftForeArm");
    require(contains(json, "\"name\": \"LeftHandMiddle4\""), "Skeleton GLB missing finger tip");
    require(!contains(json, "\"name\": \"HeadTop_End\""), "Skeleton GLB should omit HeadTop_End");
    require(!contains(json, "ToeBase\""), "Skeleton GLB should rename toe endpoints");
    require(contains(json, "\"name\": \"LeftFoot_End\"") && contains(json, "\"name\": \"RightFoot_End\""), "Skeleton GLB missing foot tails");
    require(contains(json, "\"path\": \"translation\"") && contains(json, "\"path\": \"rotation\""), "Skeleton GLB missing animation channels");
    require(countOccurrences(json, "\"path\": \"translation\"") == 1, "Skeleton GLB should only animate root translation");
    require(contains(json, "\"node\": 1, \"path\": \"translation\""), "Spine should carry root translation");
    require(!contains(json, "\"node\": 0, \"path\": \"translation\""), "root should not carry translation keys");
    require(!contains(json, "\"node\": 0, \"path\": \"rotation\""), "root should not carry rotation keys");

    std::error_code ec;
    std::filesystem::remove_all(outputDir, ec);
}

} // namespace ovtr::test

#endif
