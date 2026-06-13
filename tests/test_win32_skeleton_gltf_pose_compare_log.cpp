#ifdef _WIN32

#include "TestCases.h"
#include "TestSupport.h"

#include "math/QuaternionUtils.h"
#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonGltfExporter.h"
#include "platform/win32/SkeletonGltfPoseCompareLog.h"
#include "platform/win32/SkeletonPose.h"
#include "platform/win32/SkeletonRecordingTypes.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace ovtr::test {
namespace {

std::array<float, 4> axisAngle(const float x, const float y, const float z, const float degrees)
{
    constexpr float kPi = 3.14159265358979323846f;
    const float radians = degrees * kPi / 180.0f;
    const float s = std::sin(radians * 0.5f);
    return {x * s, y * s, z * s, std::cos(radians * 0.5f)};
}

std::string textFile(const std::filesystem::path& path)
{
    const std::vector<std::uint8_t> bytes = readBinaryFile(path);
    return std::string(bytes.begin(), bytes.end());
}

bool contains(const std::string& text, const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}

std::string glbJson(const std::vector<std::uint8_t>& bytes)
{
    require(bytes.size() > 20, "Skeleton GLB too small");
    require(readLittleEndianUint32(bytes, 0) == 0x46546c67u, "Skeleton GLB magic mismatch");
    const std::uint32_t jsonLength = readLittleEndianUint32(bytes, 12);
    require(readLittleEndianUint32(bytes, 16) == 0x4e4f534au, "Skeleton GLB JSON chunk mismatch");
    require(bytes.size() >= 20ULL + jsonLength, "Skeleton GLB JSON chunk truncated");
    return std::string(bytes.begin() + 20, bytes.begin() + 20 + jsonLength);
}

std::size_t glbBinOffset(const std::vector<std::uint8_t>& bytes)
{
    const std::uint32_t jsonLength = readLittleEndianUint32(bytes, 12);
    const std::size_t binHeader = 20ULL + jsonLength;
    require(bytes.size() >= binHeader + 8ULL, "Skeleton GLB missing BIN chunk");
    require(readLittleEndianUint32(bytes, binHeader + 4ULL) == 0x004e4942u, "Skeleton GLB BIN chunk mismatch");
    return binHeader + 8ULL;
}

float readFloat32(const std::vector<std::uint8_t>& bytes, const std::size_t offset)
{
    const std::uint32_t bits = readLittleEndianUint32(bytes, offset);
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

} // namespace

void testWin32SkeletonGltfPoseCompareLog()
{
    using namespace ovtr::win32;

    const BodyProfile profile;
    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(profile);
    SkeletonPose restPose = makeRestSkeletonPose(rest);
    SkeletonPose inverted = restPose;
    inverted.timeSeconds = 1.0 / 60.0;
    inverted.bones[kProfileJointHips].localRotation = axisAngle(1.0f, 0.0f, 0.0f, 180.0f);
    inverted.bones[kProfileJointLeftUpLeg].localRotation = axisAngle(0.0f, 0.0f, 1.0f, 35.0f);
    inverted.bones[kProfileJointRightUpLeg].localRotation = axisAngle(0.0f, 0.0f, 1.0f, -35.0f);

    SkeletonRecordingClip clip;
    clip.actorId = 7;
    clip.profile = profile;
    clip.actorName = L"pose_compare";
    clip.frames.push_back(SkeletonRecordingFrame{0.0, restPose});
    clip.frames.push_back(SkeletonRecordingFrame{inverted.timeSeconds, inverted});

    const std::filesystem::path outputDir = std::filesystem::current_path() / ".tmp_ovtr_pose_compare_log";
    const std::filesystem::path outputPath = outputDir / "actor_skeleton.glb";
    std::string error;
    require(exportSkeletonRecordingToGlb(clip, outputPath, error), "Skeleton GLB export failed: " + error);

    const std::filesystem::path csvPath = outputDir / "actor_skeleton_local_pose_compare.csv";
    std::error_code ec;
    const bool csvExists = std::filesystem::exists(csvPath, ec);
    require(!ec, "Failed to inspect pose compare CSV path");
    if (kSkeletonGltfPoseCompareCsvLogEnabled) {
        const std::string csv = textFile(csvPath);
        require(contains(csv, "frame,time,phase,joint,name,gltf_parent"), "pose compare CSV missing header");
        require(contains(csv, ",Hips,"), "pose compare CSV missing Hips row");
        require(contains(csv, ",LeftUpLeg,"), "pose compare CSV missing LeftUpLeg row");
        require(contains(csv, ",RightUpLeg,"), "pose compare CSV missing RightUpLeg row");
    } else {
        require(!csvExists, "pose compare CSV should be disabled by default");
    }

    const std::vector<std::uint8_t> glbBytes = readBinaryFile(outputPath);
    const std::string json = glbJson(glbBytes);
    require(contains(json, "\"count\": 2, \"type\": \"SCALAR\""), "Skeleton GLB should store recorded time keys only");
    const std::size_t binOffset = glbBinOffset(glbBytes);
    require(std::abs(readFloat32(glbBytes, binOffset) - 0.0f) < 0.0001f, "Skeleton GLB first recorded key time mismatch");
    require(std::abs(readFloat32(glbBytes, binOffset + 4ULL) - (1.0f / 60.0f)) < 0.0001f, "Skeleton GLB second recorded key time mismatch");

    std::filesystem::remove_all(outputDir, ec);
}

} // namespace ovtr::test

#endif
