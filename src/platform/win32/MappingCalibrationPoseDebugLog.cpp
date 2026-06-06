#include "platform/win32/MappingCalibrationPoseDebugLog.h"

#include "platform/win32/MappingCalibrationTargetOffsetDebugLog.h"
#include "platform/win32/MappingTransformMath.h"
#include "platform/win32/SkeletonPose.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>

namespace ovtr::win32 {
namespace {

constexpr int kPostSolveFrameLimit = 1800;

struct DebugState {
    std::filesystem::path path;
    std::filesystem::path ikPath;
    std::uint32_t actorId = 0;
    int frame = 0;
    bool active = false;
};

DebugState g_debug;

Vec3 cross(const Vec3 a, const Vec3 b) noexcept
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
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

Vec3 jointAxisY(const ProfileSkeletonJoints& joints, const int joint) noexcept
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

Vec3 renderedAxisX(
    const ProfileSkeletonJoints& joints,
    const std::array<Vec3, kProfileSkeletonJointCount>& sideAxes,
    const std::array<Vec3, kProfileSkeletonJointCount>& forwardAxes,
    const int joint
) noexcept {
    const int child = firstChildIndex(joints, joint);
    const int segment = child >= 0 ? child : joint;
    const Vec3 y = jointAxisY(joints, joint);
    Vec3 x = subMappingVec3(
        sideAxes[static_cast<std::size_t>(segment)],
        scaleMappingVec3(y, dotMappingVec3(sideAxes[static_cast<std::size_t>(segment)], y))
    );
    return normalizeMappingVec3Or(
        x,
        normalizeMappingVec3Or(cross(y, forwardAxes[static_cast<std::size_t>(segment)]), Vec3{1.0f, 0.0f, 0.0f})
    );
}

Vec3 ownAxisX(
    const ProfileSkeletonJoints& joints,
    const std::array<Vec3, kProfileSkeletonJointCount>& sideAxes,
    const std::array<Vec3, kProfileSkeletonJointCount>& forwardAxes,
    const int joint
) noexcept {
    const Vec3 y = jointAxisY(joints, joint);
    Vec3 x = subMappingVec3(
        sideAxes[static_cast<std::size_t>(joint)],
        scaleMappingVec3(y, dotMappingVec3(sideAxes[static_cast<std::size_t>(joint)], y))
    );
    return normalizeMappingVec3Or(
        x,
        normalizeMappingVec3Or(cross(y, forwardAxes[static_cast<std::size_t>(joint)]), Vec3{1.0f, 0.0f, 0.0f})
    );
}

double angleDegrees(const Vec3 a, const Vec3 b)
{
    const double dot = std::clamp(static_cast<double>(dotMappingVec3(
        normalizeMappingVec3Or(a, Vec3{1.0f, 0.0f, 0.0f}),
        normalizeMappingVec3Or(b, Vec3{1.0f, 0.0f, 0.0f})
    )), -1.0, 1.0);
    return std::acos(dot) * 57.29577951308232;
}

std::filesystem::path exportDir()
{
    return std::filesystem::path(L"C:\\Desktop\\export");
}

std::wstring timestamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
    localtime_s(&local, &time);
    wchar_t buffer[32]{};
    std::wcsftime(buffer, std::size(buffer), L"%Y%m%d_%H%M%S", &local);
    return buffer;
}

void writeVec(std::ofstream& out, const Vec3 v)
{
    out << v.x << ',' << v.y << ',' << v.z;
}

Vec3 projectedDirection(const Vec3 axis, const Vec3 value) noexcept
{
    return normalizeMappingVec3Or(
        subMappingVec3(value, scaleMappingVec3(axis, dotMappingVec3(value, axis))),
        Vec3{0.0f, 0.0f, 0.0f}
    );
}

void writeIkRow(
    std::ofstream& out,
    const MappingActor& actor,
    const char* phase,
    const int frame,
    const char* limb,
    const int rootIndex,
    const int midIndex,
    const int endIndex,
    const int poleIndex
) {
    const auto& joints = actor.liveJoints;
    const Vec3 root = joints[rootIndex].positionMeters;
    const Vec3 mid = joints[midIndex].positionMeters;
    const Vec3 end = joints[endIndex].positionMeters;
    const MappingDebugPole& pole = actor.liveDebugPoles[static_cast<std::size_t>(poleIndex)];
    const Vec3 axis = normalizeMappingVec3Or(subMappingVec3(end, root), Vec3{1.0f, 0.0f, 0.0f});
    const Vec3 bendDir = projectedDirection(axis, subMappingVec3(mid, root));
    const Vec3 poleDir = projectedDirection(axis, subMappingVec3(pole.pole, root));
    out << frame << ',' << phase << ',' << actor.id << ',' << limb << ',' << pole.valid << ','
        << pole.fallback << ',' << pole.limited << ',' << distanceMappingVec3(root, end) << ','
        << distanceMappingVec3(root, mid) << ',' << distanceMappingVec3(mid, end) << ','
        << angleDegrees(bendDir, poleDir) << ',';
    writeVec(out, root); out << ','; writeVec(out, mid); out << ','; writeVec(out, end); out << ',';
    writeVec(out, pole.pole); out << ','; writeVec(out, bendDir); out << ','; writeVec(out, poleDir); out << '\n';
}

void writeIkRows(std::ofstream& out, const MappingActor& actor, const char* phase, const int frame)
{
    writeIkRow(out, actor, phase, frame, "LeftArm", kProfileJointLeftShoulder, kProfileJointLeftArm,
        kProfileJointLeftForeArm, 0);
    writeIkRow(out, actor, phase, frame, "RightArm", kProfileJointRightShoulder, kProfileJointRightArm,
        kProfileJointRightForeArm, 1);
    writeIkRow(out, actor, phase, frame, "LeftLeg", kProfileJointLeftUpLeg, kProfileJointLeftLeg,
        kProfileJointLeftFoot, 2);
    writeIkRow(out, actor, phase, frame, "RightLeg", kProfileJointRightUpLeg, kProfileJointRightLeg,
        kProfileJointRightFoot, 3);
}

void writeRows(std::ofstream& out, const MappingActor& actor, const char* phase, const int frame)
{
    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(actor.profile);
    const auto restSide = computeSkeletonPoseWorldSideAxes(rest, makeRestSkeletonPose(rest));
    const auto restForward = computeSkeletonPoseWorldForwardAxes(rest, makeRestSkeletonPose(rest));
    const auto liveSide = computeSkeletonPoseWorldSideAxes(rest, actor.liveSkeletonPose);
    const auto liveForward = computeSkeletonPoseWorldForwardAxes(rest, actor.liveSkeletonPose);
    for (const int joint : {kProfileJointLeftShoulder, kProfileJointLeftArm, kProfileJointLeftForeArm, kProfileJointLeftHand,
             kProfileJointRightShoulder, kProfileJointRightArm, kProfileJointRightForeArm, kProfileJointRightHand}) {
        const Vec3 restX = renderedAxisX(rest, restSide, restForward, joint);
        const Vec3 liveX = renderedAxisX(actor.liveJoints, liveSide, liveForward, joint);
        const Vec3 restOwnX = ownAxisX(rest, restSide, restForward, joint);
        const Vec3 liveOwnX = ownAxisX(actor.liveJoints, liveSide, liveForward, joint);
        const Vec3 restY = jointAxisY(rest, joint);
        const Vec3 liveY = jointAxisY(actor.liveJoints, joint);
        const Vec3 restZ = normalizeMappingVec3Or(cross(restX, restY), Vec3{0.0f, 0.0f, 1.0f});
        const Vec3 liveZ = normalizeMappingVec3Or(cross(liveX, liveY), Vec3{0.0f, 0.0f, 1.0f});
        out << frame << ',' << phase << ',' << actor.id << ',' << joint << ',' << rest[static_cast<std::size_t>(joint)].name << ',';
        writeVec(out, restX); out << ','; writeVec(out, liveX); out << ',' << angleDegrees(restX, liveX) << ',';
        writeVec(out, restOwnX); out << ','; writeVec(out, liveOwnX); out << ',' << angleDegrees(restOwnX, liveOwnX) << ',';
        writeVec(out, restY); out << ','; writeVec(out, liveY); out << ',' << angleDegrees(restY, liveY) << ',';
        writeVec(out, restZ); out << ','; writeVec(out, liveZ); out << ',' << angleDegrees(restZ, liveZ) << '\n';
    }
}

bool openAppend(std::ofstream& out, const std::filesystem::path& path)
{
    out.open(path, std::ios::app);
    return static_cast<bool>(out);
}

} // namespace

bool startMappingCalibrationPoseDebugLog(const MappingActor& actor, std::wstring& message)
{
    std::error_code ec;
    std::filesystem::create_directories(exportDir(), ec);
    if (ec) {
        message = L"Calibration pose debug log directory failed: " + exportDir().wstring();
        return false;
    }
    const std::wstring stamp = timestamp();
    g_debug = DebugState{
        exportDir() / (L"mapping_calibration_pose_debug_" + stamp + L".csv"),
        exportDir() / (L"mapping_calibration_ik_debug_" + stamp + L".csv"),
        actor.id,
        0,
        true
    };
    std::ofstream out(g_debug.path);
    std::ofstream ikOut(g_debug.ikPath);
    if (!out || !ikOut) {
        message = L"Calibration pose debug log failed: " + g_debug.path.wstring();
        g_debug.active = false;
        return false;
    }
    out << std::fixed << std::setprecision(6);
    out << "frame,phase,actor_id,joint,name,rest_xx,rest_xy,rest_xz,live_xx,live_xy,live_xz,x_delta_deg,"
        << "rest_own_xx,rest_own_xy,rest_own_xz,live_own_xx,live_own_xy,live_own_xz,own_x_delta_deg,"
        << "rest_yx,rest_yy,rest_yz,live_yx,live_yy,live_yz,y_delta_deg,"
        << "rest_zx,rest_zy,rest_zz,live_zx,live_zy,live_zz,z_delta_deg\n";
    writeRows(out, actor, "capture_rest", -1);
    ikOut << std::fixed << std::setprecision(6);
    ikOut << "frame,phase,actor_id,limb,pole_valid,pole_fallback,pole_limited,root_end_distance,"
        << "root_mid_distance,mid_end_distance,bend_to_pole_angle_deg,root_x,root_y,root_z,mid_x,mid_y,mid_z,"
        << "end_x,end_y,end_z,pole_x,pole_y,pole_z,bend_dir_x,bend_dir_y,bend_dir_z,pole_dir_x,pole_dir_y,pole_dir_z\n";
    writeIkRows(ikOut, actor, "capture_rest", -1);
    const std::filesystem::path offsetPath =
        exportDir() / (L"mapping_calibration_target_offset_debug_" + stamp + L".csv");
    startMappingCalibrationTargetOffsetDebugLog(actor, offsetPath);
    message = L"Calibration pose debug logs: " + g_debug.path.wstring() + L" / " + g_debug.ikPath.wstring() +
        L" / " + offsetPath.wstring();
    return true;
}

void appendMappingCalibrationPoseDebugLog(const MappingActor& actor)
{
    if (!g_debug.active || actor.id != g_debug.actorId || g_debug.frame >= kPostSolveFrameLimit) {
        return;
    }
    std::ofstream out;
    if (!openAppend(out, g_debug.path)) {
        g_debug.active = false;
        return;
    }
    out << std::fixed << std::setprecision(6);
    writeRows(out, actor, "post_solve", g_debug.frame++);
    std::ofstream ikOut;
    if (openAppend(ikOut, g_debug.ikPath)) {
        ikOut << std::fixed << std::setprecision(6);
        writeIkRows(ikOut, actor, "post_solve", g_debug.frame - 1);
    }
    appendMappingCalibrationTargetOffsetDebugLog(actor);
}

} // namespace ovtr::win32
