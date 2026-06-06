#include "platform/win32/SkeletonGltfPoseCompareLog.h"

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

constexpr double kRadiansToDegrees = 57.29577951308232;

struct Basis {
    Vec3 x{};
    Vec3 y{};
    Vec3 z{};
};

Vec3 crossVec(const Vec3 a, const Vec3 b) noexcept
{
    return Vec3{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

std::array<float, 4> mul(const std::array<float, 4>& a, const std::array<float, 4>& b)
{
    return ovtr::normalizeQuaternion(ovtr::multiplyQuaternion(a, b));
}

Vec3 rotated(const std::array<float, 4>& q, const Vec3 axis)
{
    const std::array<float, 3> out = ovtr::rotatePositionByQuaternion(q, {axis.x, axis.y, axis.z});
    return Vec3{out[0], out[1], out[2]};
}

double angleBetween(const Vec3 a, const Vec3 b)
{
    const double dot = std::clamp(
        static_cast<double>(dotMappingVec3(normalizeMappingVec3Or(a, Vec3{1.0f, 0.0f, 0.0f}), normalizeMappingVec3Or(b, Vec3{1.0f, 0.0f, 0.0f}))),
        -1.0,
        1.0
    );
    return std::acos(dot) * kRadiansToDegrees;
}

double rotationDeltaDegrees(const std::array<float, 4>& a, const std::array<float, 4>& b)
{
    const double dot = std::clamp(std::abs(
        static_cast<double>(a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3])
    ), 0.0, 1.0);
    return 2.0 * std::acos(dot) * kRadiansToDegrees;
}

int firstChildIndex(const ProfileSkeletonJoints& joints, const int parent) noexcept
{
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        if (joints[static_cast<std::size_t>(joint)].parentIndex == parent) {
            return joint;
        }
    }
    return -1;
}

Vec3 jointMainAxis(const ProfileSkeletonJoints& joints, const int joint) noexcept
{
    const int child = firstChildIndex(joints, joint);
    if (child >= 0) {
        return normalizeMappingVec3Or(
            subMappingVec3(joints[static_cast<std::size_t>(child)].positionMeters, joints[static_cast<std::size_t>(joint)].positionMeters),
            Vec3{0.0f, 1.0f, 0.0f}
        );
    }
    const int parent = joints[static_cast<std::size_t>(joint)].parentIndex;
    return parent >= 0
        ? normalizeMappingVec3Or(subMappingVec3(joints[static_cast<std::size_t>(joint)].positionMeters, joints[static_cast<std::size_t>(parent)].positionMeters), Vec3{0.0f, 1.0f, 0.0f})
        : Vec3{0.0f, 1.0f, 0.0f};
}

Basis viewportBasis(
    const ProfileSkeletonJoints& joints,
    const std::array<Vec3, kProfileSkeletonJointCount>& sideAxes,
    const std::array<Vec3, kProfileSkeletonJointCount>& forwardAxes,
    const int joint
) {
    const int child = firstChildIndex(joints, joint);
    const int segment = child >= 0 ? child : joint;
    const Vec3 y = jointMainAxis(joints, joint);
    Vec3 x = subMappingVec3(
        sideAxes[static_cast<std::size_t>(segment)],
        scaleMappingVec3(y, dotMappingVec3(sideAxes[static_cast<std::size_t>(segment)], y))
    );
    x = normalizeMappingVec3Or(x, normalizeMappingVec3Or(crossVec(y, forwardAxes[static_cast<std::size_t>(segment)]), Vec3{1.0f, 0.0f, 0.0f}));
    return Basis{x, y, normalizeMappingVec3Or(crossVec(x, y), Vec3{0.0f, 0.0f, 1.0f})};
}

std::array<std::array<float, 4>, kProfileSkeletonJointCount> gltfWorldRotations(
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& pose
) {
    std::array<std::array<float, 4>, kProfileSkeletonJointCount> world{};
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        const int parent = skeletonGltfParentIndex(rest, joint);
        const auto& local = pose.bones[static_cast<std::size_t>(joint)].localRotation;
        world[static_cast<std::size_t>(joint)] = parent >= 0 ? mul(world[static_cast<std::size_t>(parent)], local) : local;
    }
    return world;
}

std::filesystem::path comparePathFor(const std::filesystem::path& glbPath)
{
    std::filesystem::path out = glbPath;
    out.replace_filename(glbPath.stem().wstring() + L"_local_pose_compare.csv");
    return out;
}

void writeVec(std::ofstream& out, const Vec3 value)
{
    out << value.x << ',' << value.y << ',' << value.z;
}

void writeQuat(std::ofstream& out, const std::array<float, 4>& q)
{
    out << q[0] << ',' << q[1] << ',' << q[2] << ',' << q[3];
}

void writeRow(
    std::ofstream& out,
    const double time,
    const int frame,
    const char* phase,
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& sourcePose,
    const SkeletonPose& gltfPose,
    const std::array<std::array<float, 4>, kProfileSkeletonJointCount>& gltfWorld,
    const Basis sourceBasis,
    const int joint
) {
    const std::size_t index = static_cast<std::size_t>(joint);
    const Vec3 gltfX = rotated(gltfWorld[index], Vec3{1.0f, 0.0f, 0.0f});
    const Vec3 gltfY = rotated(gltfWorld[index], Vec3{0.0f, 1.0f, 0.0f});
    const Vec3 gltfZ = rotated(gltfWorld[index], Vec3{0.0f, 0.0f, 1.0f});
    const auto& sourceLocal = sourcePose.bones[index].localRotation;
    const auto& gltfLocal = gltfPose.bones[index].localRotation;
    out << frame << ',' << time << ',' << phase << ',' << joint << ',' << rest[index].name << ','
        << skeletonGltfParentIndex(rest, joint) << ',';
    writeQuat(out, sourceLocal);
    out << ',';
    writeQuat(out, gltfLocal);
    out << ',' << rotationDeltaDegrees(sourceLocal, gltfLocal) << ',';
    writeVec(out, sourceBasis.x);
    out << ',';
    writeVec(out, gltfX);
    out << ',' << angleBetween(sourceBasis.x, gltfX) << ',';
    writeVec(out, sourceBasis.z);
    out << ',';
    writeVec(out, gltfZ);
    out << ',' << angleBetween(sourceBasis.z, gltfZ) << ',';
    writeVec(out, sourceBasis.y);
    out << ',';
    writeVec(out, gltfY);
    out << ',' << angleBetween(sourceBasis.y, gltfY) << '\n';
}

void writePoseRows(
    std::ofstream& out,
    const double time,
    const int frame,
    const char* phase,
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& sourcePose,
    const SkeletonPose& gltfPose
) {
    const ProfileSkeletonJoints sourceJoints = computeSkeletonPoseWorldJoints(rest, sourcePose);
    const auto gltfWorld = gltfWorldRotations(rest, gltfPose);
    const auto sourceSide = computeSkeletonPoseWorldSideAxes(rest, sourcePose);
    const auto sourceForward = computeSkeletonPoseWorldForwardAxes(rest, sourcePose);
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        if (!isSkeletonGltfExportedJoint(joint)) {
            continue;
        }
        writeRow(out, time, frame, phase, rest, sourcePose, gltfPose, gltfWorld, viewportBasis(sourceJoints, sourceSide, sourceForward, joint), joint);
    }
}

} // namespace

bool writeSkeletonGltfPoseCompareLog(
    const SkeletonRecordingClip& clip,
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& bindPose,
    const std::vector<SkeletonPose>& gltfPoses,
    const std::filesystem::path& glbPath,
    std::string& error
) {
    const std::filesystem::path path = comparePathFor(glbPath);
    std::ofstream out(path);
    if (!out) {
        error = "Failed to write skeleton pose compare log: " + path.string();
        return false;
    }
    out << std::fixed << std::setprecision(6);
    out << "frame,time,phase,joint,name,gltf_parent,"
        << "source_lqx,source_lqy,source_lqz,source_lqw,gltf_lqx,gltf_lqy,gltf_lqz,gltf_lqw,local_delta_deg,"
        << "source_xx,source_xy,source_xz,gltf_xx,gltf_xy,gltf_xz,x_delta_deg,"
        << "source_zx,source_zy,source_zz,gltf_zx,gltf_zy,gltf_zz,z_delta_deg,"
        << "source_yx,source_yy,source_yz,gltf_yx,gltf_yy,gltf_yz,y_delta_deg\n";
    const SkeletonPose restPose = makeRestSkeletonPose(rest);
    writePoseRows(out, 0.0, -1, "bind", rest, restPose, bindPose);
    for (std::size_t frame = 0; frame < clip.frames.size() && frame < gltfPoses.size(); ++frame) {
        writePoseRows(out, clip.frames[frame].timeSeconds, static_cast<int>(frame), "anim", rest, clip.frames[frame].pose, gltfPoses[frame]);
    }
    return true;
}

} // namespace ovtr::win32
