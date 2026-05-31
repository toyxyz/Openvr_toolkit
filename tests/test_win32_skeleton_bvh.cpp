#ifdef _WIN32

#include "TestCases.h"
#include "TestSupport.h"

#include "platform/win32/AppProfileState.h"
#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonBvhExporter.h"
#include "platform/win32/SkeletonRecording.h"

#include <filesystem>
#include <string>
#include <system_error>

namespace ovtr::test {
namespace {

bool contains(const std::string& text, const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}

} // namespace

void testWin32SkeletonBvhExport()
{
    using namespace ovtr::win32;

    MappingActor actor;
    actor.id = 42;
    actor.calibrated = true;
    actor.profile.name = L"bvh_actor";
    actor.liveJoints = buildProfileSkeletonJoints(actor.profile);
    actor.liveJointsValid = true;

    SkeletonRecordingClip clip;
    beginSkeletonRecording(clip, actor);
    clip.frames.push_back(SkeletonRecordingFrame{0.0, actor.liveJoints});

    actor.liveJoints[kProfileJointLeftForeArm].positionMeters.z += 0.10f;
    actor.liveJoints[kProfileJointLeftHand].positionMeters.z += 0.10f;
    clip.frames.push_back(SkeletonRecordingFrame{1.0 / 60.0, actor.liveJoints});
    finishSkeletonRecording(clip);

    const std::filesystem::path outputDir =
        std::filesystem::current_path() / ".tmp_ovtr_skeleton_bvh";
    const std::filesystem::path outputPath = outputDir / "actor.bvh";
    std::string error;
    require(exportSkeletonRecordingToBvh(clip, outputPath, error), "BVH export failed: " + error);

    const std::string text = readTextFile(outputPath);
    require(contains(text, "HIERARCHY\nROOT Hips"), "BVH hierarchy missing Hips root");
    require(contains(text, "JOINT LeftArm"), "BVH hierarchy missing Mixamo LeftArm");
    require(contains(text, "JOINT LeftHandMiddle4"), "BVH hierarchy missing Mixamo finger tip");
    require(contains(text, "CHANNELS 6 Xposition Yposition Zposition Xrotation Yrotation Zrotation"),
        "BVH root channels mismatch");
    require(contains(text, "MOTION\nFrames: 2\nFrame Time: 0.016667"),
        "BVH motion header mismatch");

    std::error_code ec;
    std::filesystem::remove_all(outputDir, ec);
}

} // namespace ovtr::test

#endif
