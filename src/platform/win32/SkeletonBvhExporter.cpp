#include "platform/win32/SkeletonBvhExporter.h"

#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonBvhMath.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace ovtr::win32 {
namespace {

constexpr float kMetersToCentimeters = 100.0f;
constexpr double kFallbackFrameTimeSeconds = 1.0 / 60.0;

using ChildLists = std::array<std::vector<int>, kProfileSkeletonJointCount>;

ChildLists buildChildLists(const ProfileSkeletonJoints& joints)
{
    ChildLists children;
    for (int index = 0; index < kProfileSkeletonJointCount; ++index) {
        const int parent = joints[static_cast<std::size_t>(index)].parentIndex;
        if (parent >= 0 && parent < kProfileSkeletonJointCount) {
            children[static_cast<std::size_t>(parent)].push_back(index);
        }
    }
    return children;
}

std::string indent(const int depth)
{
    return std::string(static_cast<std::size_t>(depth) * 2U, ' ');
}

Vec3 jointOffsetCm(const ProfileSkeletonJoints& joints, const int index) noexcept
{
    const ProfileSkeletonJoint& joint = joints[static_cast<std::size_t>(index)];
    if (joint.parentIndex < 0) {
        return Vec3{};
    }
    const Vec3 parent = joints[static_cast<std::size_t>(joint.parentIndex)].positionMeters;
    return Vec3{
        (joint.positionMeters.x - parent.x) * kMetersToCentimeters,
        (joint.positionMeters.y - parent.y) * kMetersToCentimeters,
        (joint.positionMeters.z - parent.z) * kMetersToCentimeters
    };
}

void writeOffset(std::ostream& out, const Vec3 offset)
{
    out << "OFFSET " << offset.x << ' ' << offset.y << ' ' << offset.z << '\n';
}

void writeJoint(
    std::ostream& out,
    const ProfileSkeletonJoints& joints,
    const ChildLists& children,
    const int index,
    const int depth
) {
    const bool root = joints[static_cast<std::size_t>(index)].parentIndex < 0;
    out << indent(depth) << (root ? "ROOT " : "JOINT ")
        << joints[static_cast<std::size_t>(index)].name << '\n';
    out << indent(depth) << "{\n";
    out << indent(depth + 1);
    writeOffset(out, jointOffsetCm(joints, index));
    if (root) {
        out << indent(depth + 1)
            << "CHANNELS 6 Xposition Yposition Zposition Xrotation Yrotation Zrotation\n";
    } else {
        out << indent(depth + 1) << "CHANNELS 3 Xrotation Yrotation Zrotation\n";
    }
    const std::vector<int>& jointChildren = children[static_cast<std::size_t>(index)];
    for (const int child : jointChildren) {
        writeJoint(out, joints, children, child, depth + 1);
    }
    if (jointChildren.empty()) {
        out << indent(depth + 1) << "End Site\n";
        out << indent(depth + 1) << "{\n";
        out << indent(depth + 2) << "OFFSET 0 0 0\n";
        out << indent(depth + 1) << "}\n";
    }
    out << indent(depth) << "}\n";
}

void appendFrameValues(
    std::ostream& out,
    const ProfileSkeletonJoints& rest,
    const ChildLists& children,
    const SkeletonBvhRotationFrame& frame,
    const int index,
    bool& firstValue
) {
    const auto writeValue = [&](const float value) {
        if (!firstValue) {
            out << ' ';
        }
        firstValue = false;
        out << value;
    };
    if (rest[static_cast<std::size_t>(index)].parentIndex < 0) {
        writeValue(frame.rootPositionCm[0]);
        writeValue(frame.rootPositionCm[1]);
        writeValue(frame.rootPositionCm[2]);
    }
    const std::array<float, 3>& euler = frame.eulerDegrees[static_cast<std::size_t>(index)];
    writeValue(euler[0]);
    writeValue(euler[1]);
    writeValue(euler[2]);
    for (const int child : children[static_cast<std::size_t>(index)]) {
        appendFrameValues(out, rest, children, frame, child, firstValue);
    }
}

double frameTimeSeconds(const SkeletonRecordingClip& clip) noexcept
{
    if (clip.frames.size() < 2) {
        return kFallbackFrameTimeSeconds;
    }
    const double duration = clip.frames.back().timeSeconds - clip.frames.front().timeSeconds;
    const double frameTime = duration / static_cast<double>(clip.frames.size() - 1U);
    return frameTime > 0.0 ? frameTime : kFallbackFrameTimeSeconds;
}

} // namespace

bool exportSkeletonRecordingToBvh(
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
            error = "Failed to create BVH export directory: " + ec.message();
            return false;
        }
    }

    std::ofstream out(outputPath, std::ios::binary);
    if (!out) {
        error = "Failed to open BVH output file.";
        return false;
    }

    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(clip.profile);
    const ChildLists children = buildChildLists(rest);
    out << std::fixed << std::setprecision(6);
    out << "HIERARCHY\n";
    writeJoint(out, rest, children, kProfileJointHips, 0);
    out << "MOTION\n";
    out << "Frames: " << clip.frames.size() << '\n';
    out << "Frame Time: " << frameTimeSeconds(clip) << '\n';

    for (const SkeletonRecordingFrame& recordedFrame : clip.frames) {
        const SkeletonBvhRotationFrame frame = makeSkeletonBvhRotationFrame(rest, recordedFrame.joints);
        bool firstValue = true;
        appendFrameValues(out, rest, children, frame, kProfileJointHips, firstValue);
        out << '\n';
    }

    if (!out) {
        error = "Failed while writing BVH output file.";
        return false;
    }
    return true;
}

} // namespace ovtr::win32
