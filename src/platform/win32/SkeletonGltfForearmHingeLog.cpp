#include "platform/win32/SkeletonGltfForearmHingeLog.h"

#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"
#include "platform/win32/MappingTransformMath.h"
#include "platform/win32/SkeletonGltfHierarchy.h"
#include "platform/win32/SkeletonPose.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>

namespace ovtr::win32 {
namespace {

constexpr int kFirstFrame = 2900;
constexpr int kLastFrame = 3000;
constexpr double kRadiansToDegrees = 57.29577951308232;

struct Axes {
    Vec3 x{};
    Vec3 y{};
    Vec3 z{};
};

Vec3 crossVec(const Vec3 a, const Vec3 b) noexcept
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

Vec3 rotated(const std::array<float, 4>& q, const Vec3 axis) noexcept
{
    const auto out = ovtr::rotatePositionByQuaternion(q, {axis.x, axis.y, axis.z});
    return {out[0], out[1], out[2]};
}

double angleBetween(const Vec3 a, const Vec3 b)
{
    const double dot = std::clamp(static_cast<double>(dotMappingVec3(
        normalizeMappingVec3Or(a, {1.0f, 0.0f, 0.0f}),
        normalizeMappingVec3Or(b, {1.0f, 0.0f, 0.0f})
    )), -1.0, 1.0);
    return std::acos(dot) * kRadiansToDegrees;
}

std::array<std::array<float, 4>, kProfileSkeletonJointCount> gltfWorldRotations(
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& pose
) {
    std::array<std::array<float, 4>, kProfileSkeletonJointCount> world{};
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        const int parent = skeletonGltfParentIndex(rest, joint);
        const auto& local = pose.bones[static_cast<std::size_t>(joint)].localRotation;
        world[static_cast<std::size_t>(joint)] = parent >= 0
            ? ovtr::normalizeQuaternion(ovtr::multiplyQuaternion(world[static_cast<std::size_t>(parent)], local))
            : local;
    }
    return world;
}

Axes worldAxes(const std::array<float, 4>& rotation) noexcept
{
    return {
        rotated(rotation, {1.0f, 0.0f, 0.0f}),
        rotated(rotation, {0.0f, 1.0f, 0.0f}),
        rotated(rotation, {0.0f, 0.0f, 1.0f})
    };
}

Axes sourceAxes(
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& pose,
    const int joint
) {
    const auto world = computeSkeletonPoseWorldRotations(rest, pose);
    const auto sideAxes = computeSkeletonPoseWorldSideAxes(rest, pose);
    return {sideAxes[static_cast<std::size_t>(joint)], rotated(world[static_cast<std::size_t>(joint)], {0.0f, 1.0f, 0.0f}),
        rotated(world[static_cast<std::size_t>(joint)], {0.0f, 0.0f, 1.0f})};
}

Vec3 hingeAxis(const ProfileSkeletonJoints& joints, const bool left) noexcept
{
    const int shoulder = left ? kProfileJointLeftShoulder : kProfileJointRightShoulder;
    const int elbow = left ? kProfileJointLeftArm : kProfileJointRightArm;
    const int wrist = left ? kProfileJointLeftForeArm : kProfileJointRightForeArm;
    const Vec3 upper = subMappingVec3(joints[elbow].positionMeters, joints[shoulder].positionMeters);
    const Vec3 lower = subMappingVec3(joints[wrist].positionMeters, joints[elbow].positionMeters);
    return normalizeMappingVec3Or(crossVec(upper, lower), {0.0f, 0.0f, 0.0f});
}

std::filesystem::path logPathFor(const std::filesystem::path& glbPath)
{
    std::filesystem::path out = glbPath;
    out.replace_filename(glbPath.stem().wstring() + L"_forearm_hinge_2900_3000.csv");
    return out;
}

void writeVec(std::ofstream& out, const Vec3 v)
{
    out << v.x << ',' << v.y << ',' << v.z;
}

void writeRow(
    std::ofstream& out,
    const int frame,
    const double time,
    const char* side,
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& sourcePose,
    const std::array<std::array<float, 4>, kProfileSkeletonJointCount>& gltfWorld,
    const int joint
) {
    const ProfileSkeletonJoints sourceJoints = computeSkeletonPoseWorldJoints(rest, sourcePose);
    const Vec3 hinge = hingeAxis(sourceJoints, side[0] == 'L');
    const Axes src = sourceAxes(rest, sourcePose, joint);
    const Axes gltf = worldAxes(gltfWorld[static_cast<std::size_t>(joint)]);
    out << frame << ',' << time << ',' << side << ',' << joint << ',' << rest[static_cast<std::size_t>(joint)].name << ',';
    writeVec(out, hinge); out << ',';
    writeVec(out, src.x); out << ',' << angleBetween(src.x, hinge) << ',';
    writeVec(out, gltf.x); out << ',' << angleBetween(gltf.x, hinge) << ',' << angleBetween(src.x, gltf.x) << ',';
    writeVec(out, src.y); out << ',';
    writeVec(out, gltf.y); out << ',' << angleBetween(src.y, gltf.y) << ',';
    writeVec(out, src.z); out << ',';
    writeVec(out, gltf.z); out << ',' << angleBetween(src.z, gltf.z) << '\n';
}

void writeSideRows(
    std::ofstream& out,
    const int frame,
    const double time,
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& sourcePose,
    const SkeletonPose& gltfPose,
    const bool left
) {
    const auto gltfWorld = gltfWorldRotations(rest, gltfPose);
    writeRow(out, frame, time, left ? "Left" : "Right", rest, sourcePose, gltfWorld,
        left ? kProfileJointLeftArm : kProfileJointRightArm);
    writeRow(out, frame, time, left ? "Left" : "Right", rest, sourcePose, gltfWorld,
        left ? kProfileJointLeftForeArm : kProfileJointRightForeArm);
}

} // namespace

bool writeSkeletonGltfForearmHingeLog(
    const SkeletonRecordingClip& clip,
    const ProfileSkeletonJoints& rest,
    const std::vector<SkeletonPose>& gltfPoses,
    const std::filesystem::path& glbPath,
    std::string& error
) {
    const std::filesystem::path path = logPathFor(glbPath);
    std::ofstream out(path);
    if (!out) {
        error = "Failed to write skeleton forearm hinge log: " + path.string();
        return false;
    }
    out << std::fixed << std::setprecision(6);
    out << "frame,time,side,joint,name,hinge_x,hinge_y,hinge_z,"
        << "source_xx,source_xy,source_xz,source_x_to_hinge_deg,"
        << "gltf_xx,gltf_xy,gltf_xz,gltf_x_to_hinge_deg,source_gltf_x_delta_deg,"
        << "source_yx,source_yy,source_yz,gltf_yx,gltf_yy,gltf_yz,source_gltf_y_delta_deg,"
        << "source_zx,source_zy,source_zz,gltf_zx,gltf_zy,gltf_zz,source_gltf_z_delta_deg\n";
    for (std::size_t frame = kFirstFrame; frame < clip.frames.size() && frame <= kLastFrame && frame < gltfPoses.size(); ++frame) {
        writeSideRows(out, static_cast<int>(frame), clip.frames[frame].timeSeconds, rest, clip.frames[frame].pose, gltfPoses[frame], true);
        writeSideRows(out, static_cast<int>(frame), clip.frames[frame].timeSeconds, rest, clip.frames[frame].pose, gltfPoses[frame], false);
    }
    return true;
}

} // namespace ovtr::win32
