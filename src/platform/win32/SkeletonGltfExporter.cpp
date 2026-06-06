#include "platform/win32/SkeletonGltfExporter.h"

#include "export/GltfExportBuffers.h"
#include "export/GltfFileWriter.h"
#include "export/GltfJsonFormatting.h"
#include "export/GltfJsonSections.h"
#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonGltfAnimationKeys.h"
#include "platform/win32/SkeletonGltfAnimationWriter.h"
#include "platform/win32/SkeletonGltfHierarchy.h"
#include "platform/win32/SkeletonGltfPose.h"
#include "platform/win32/SkeletonGltfSkin.h"
#include "platform/win32/SkeletonPose.h"
#include "util/JsonWriter.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <ostream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace ovtr::win32 {
namespace {

constexpr int kFirstJointNode = 0;
constexpr int kSkinJointCount = kProfileSkeletonJointCount - 2;

int jointNodeIndex(const int joint) noexcept
{
    int index = kFirstJointNode;
    for (int current = 0; current < joint; ++current) {
        if (isSkeletonGltfExportedJoint(current)) {
            ++index;
        }
    }
    return index;
}

std::vector<float> frameTimes(const SkeletonRecordingClip& clip)
{
    std::vector<float> times;
    times.reserve(clip.frames.size());
    for (const SkeletonRecordingFrame& frame : clip.frames) {
        times.push_back(static_cast<float>(frame.timeSeconds));
    }
    return times;
}

int addFloatAccessor(
    std::vector<std::uint8_t>& binary,
    std::vector<ovtr::GltfExportBufferView>& views,
    std::vector<ovtr::GltfExportAccessor>& accessors,
    const std::vector<float>& values,
    const int count,
    const std::string& type
) {
    const int view = ovtr::appendGltfFloatBufferView(binary, views, values);
    return ovtr::addGltfAccessor(accessors, view, ovtr::kGltfComponentFloat, count, type);
}

std::vector<SkeletonPose> exportPoses(const SkeletonRecordingClip& clip, const ProfileSkeletonJoints& rest)
{
    std::vector<SkeletonPose> poses;
    poses.reserve(clip.frames.size());
    for (const SkeletonRecordingFrame& frame : clip.frames) {
        poses.push_back(frame.pose);
    }
    return makeSkeletonGltfExportPoses(rest, poses);
}

std::vector<float> jointTranslations(const std::vector<SkeletonPose>& poses, const int joint)
{
    std::vector<float> values;
    values.reserve(poses.size() * 3U);
    for (const SkeletonPose& pose : poses) {
        const Vec3 translation = pose.bones[static_cast<std::size_t>(joint)].localTranslationMeters;
        values.push_back(translation.x);
        values.push_back(translation.y);
        values.push_back(translation.z);
    }
    return values;
}

std::vector<float> skinJointInverseBindMatrices(const std::vector<float>& matrices)
{
    constexpr int kMatrixFloats = 16;
    std::vector<float> values;
    values.reserve(kSkinJointCount * kMatrixFloats);
    for (int joint = 1; joint < kProfileSkeletonJointCount; ++joint) {
        if (!isSkeletonGltfSkinJoint(joint)) {
            continue;
        }
        const auto begin = matrices.begin() + joint * kMatrixFloats;
        values.insert(values.end(), begin, begin + kMatrixFloats);
    }
    return values;
}

void writeChildren(std::ostream& out, const std::vector<int>& children)
{
    if (children.empty()) {
        return;
    }
    out << ", \"children\": [";
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (i != 0) {
            out << ",";
        }
        out << children[i];
    }
    out << "]";
}

std::vector<std::vector<int>> childNodes(const ProfileSkeletonJoints& rest)
{
    std::vector<std::vector<int>> children(kProfileSkeletonJointCount);
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        const int parent = skeletonGltfParentIndex(rest, joint);
        if (parent >= 0 && isSkeletonGltfExportedJoint(joint)) {
            children[static_cast<std::size_t>(parent)].push_back(jointNodeIndex(joint));
        }
    }
    return children;
}

const char* nodeName(const ProfileSkeletonJoints& rest, const int joint) noexcept
{
    if (joint == kProfileJointHips) { return "root"; }
    if (joint == kProfileJointLeftToeBase) { return "LeftFoot_End"; }
    if (joint == kProfileJointRightToeBase) { return "RightFoot_End"; }
    return rest[static_cast<std::size_t>(joint)].name;
}

void writeNodes(std::ostream& out, const ProfileSkeletonJoints& rest, const SkeletonPose& firstPose)
{
    const std::vector<std::vector<int>> children = childNodes(rest);
    out << "  \"nodes\": [\n";
    bool first = true;
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        if (!isSkeletonGltfExportedJoint(joint)) {
            continue;
        }
        const SkeletonBonePose& bone = firstPose.bones[static_cast<std::size_t>(joint)];
        out << (first ? "    " : ",\n    ");
        out << "{ \"name\": \"" << ovtr::escapeJsonString(nodeName(rest, joint)) << "\"";
        out << ", \"translation\": ";
        const std::array<float, 3> translation{
            bone.localTranslationMeters.x,
            bone.localTranslationMeters.y,
            bone.localTranslationMeters.z
        };
        ovtr::detail::writeGltfJsonFloatArray(out, translation);
        out << ", \"rotation\": ";
        ovtr::detail::writeGltfJsonFloatArray(out, bone.localRotation);
        writeChildren(out, children[static_cast<std::size_t>(joint)]);
        out << " }";
        first = false;
    }
    out << "\n  ],\n";
}

void writeSkins(std::ostream& out, const int inverseBindAccessor)
{
    out << "  \"skins\": [\n";
    out << "    { \"name\": \"rig\", \"skeleton\": " << jointNodeIndex(kProfileJointSpine)
        << ", \"inverseBindMatrices\": " << inverseBindAccessor << ", \"joints\": [";
    bool first = true;
    for (int joint = 1; joint < kProfileSkeletonJointCount; ++joint) {
        if (!isSkeletonGltfSkinJoint(joint)) {
            continue;
        }
        if (!first) {
            out << ",";
        }
        out << jointNodeIndex(joint);
        first = false;
    }
    out << "] }\n  ],\n";
}

} // namespace

bool exportSkeletonRecordingToGlb(
    const SkeletonRecordingClip& clip,
    const std::filesystem::path& outputPath,
    std::string& error
) {
    error.clear();
    if (clip.frames.empty()) {
        error = "No skeleton frames were recorded.";
        return false;
    }

    std::error_code ec;
    const std::filesystem::path parent = outputPath.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            error = "Failed to create skeleton GLB directory: " + ec.message();
            return false;
        }
    }

    std::vector<std::uint8_t> binary;
    std::vector<ovtr::GltfExportBufferView> views;
    std::vector<ovtr::GltfExportAccessor> accessors;
    const std::vector<float> times = frameTimes(clip); const int timeAccessor = addFloatAccessor(binary, views, accessors, times, static_cast<int>(times.size()), "SCALAR");
    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(clip.profile);
    const SkeletonPose bindPose = makeSkeletonGltfExportPose(rest, makeRestSkeletonPose(rest), true);
    const std::vector<SkeletonPose> poses = exportPoses(clip, rest);
    // Temporary CSV diagnostics remain in SkeletonGltfPoseCompareLog.* and
    // SkeletonGltfForearmHingeLog.* for future export-axis investigations.
    const int inverseBindAccessor = addFloatAccessor(
        binary,
        views,
        accessors,
        skinJointInverseBindMatrices(makeSkeletonInverseBindMatrices(rest, bindPose)),
        kSkinJointCount,
        "MAT4"
    );
    const int rootTranslationAccessor = addFloatAccessor(
        binary,
        views,
        accessors,
        jointTranslations(poses, kProfileJointSpine),
        static_cast<int>(clip.frames.size()),
        "VEC3"
    );
    std::array<int, kProfileSkeletonJointCount> rotationAccessors{};
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        rotationAccessors[static_cast<std::size_t>(joint)] = addFloatAccessor(
            binary,
            views,
            accessors,
            continuousJointRotationKeys(poses, joint),
            static_cast<int>(clip.frames.size()),
            "VEC4"
        );
    }
    std::array<int, kProfileSkeletonJointCount> translationAccessors{};
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        if (!isSkeletonGltfTranslationAnimatedJoint(joint)) {
            continue;
        }
        translationAccessors[static_cast<std::size_t>(joint)] = addFloatAccessor(
            binary,
            views,
            accessors,
            jointTranslations(poses, joint),
            static_cast<int>(clip.frames.size()),
            "VEC3"
        );
    }

    std::ostringstream json;
    json << "{\n";
    json << "  \"asset\": { \"version\": \"2.0\", \"generator\": \"toyxyz_openvr_toolkit\" },\n";
    json << "  \"scene\": 0,\n  \"scenes\": [{ \"nodes\": [" << jointNodeIndex(kProfileJointHips) << "] }],\n";
    writeNodes(json, rest, bindPose);
    writeSkins(json, inverseBindAccessor);
    writeSkeletonGltfAnimation(json, timeAccessor, rootTranslationAccessor, rotationAccessors, translationAccessors);
    ovtr::detail::writeGltfJsonBufferMetadata(json, views, accessors, binary.size(), "");
    json << "  \"extras\": { \"skeleton\": \"Mixamo subset\", \"frames\": " << clip.frames.size() << " }\n";
    json << "}\n";

    if (!ovtr::writeGltfGlbFile(outputPath, json.str(), binary)) {
        error = "Failed to write skeleton GLB output file.";
        return false;
    }
    return true;
}

} // namespace ovtr::win32
