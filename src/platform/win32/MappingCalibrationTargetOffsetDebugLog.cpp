#include "platform/win32/MappingCalibrationTargetOffsetDebugLog.h"

#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"
#include "platform/win32/MappingTransformMath.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>

namespace ovtr::win32 {
namespace {

constexpr int kPostSolveFrameLimit = 1800;

struct TargetOffsetDebugState {
    std::filesystem::path path;
    std::uint32_t actorId = 0;
    int frame = 0;
    bool active = false;
};

TargetOffsetDebugState g_targetOffsetDebug;

const char* roleName(const MappingTrackerRole role) noexcept
{
    switch (role) {
    case MappingTrackerRole::Head: return "Head";
    case MappingTrackerRole::Chest: return "Chest";
    case MappingTrackerRole::Pelvis: return "Pelvis";
    case MappingTrackerRole::LeftArm: return "LeftArm";
    case MappingTrackerRole::RightArm: return "RightArm";
    case MappingTrackerRole::LeftHand: return "LeftHand";
    case MappingTrackerRole::RightHand: return "RightHand";
    case MappingTrackerRole::LeftLeg: return "LeftLeg";
    case MappingTrackerRole::RightLeg: return "RightLeg";
    case MappingTrackerRole::LeftFoot: return "LeftFoot";
    case MappingTrackerRole::RightFoot: return "RightFoot";
    }
    return "Unknown";
}

double quaternionAngleDegrees(const std::array<float, 4>& left, const std::array<float, 4>& right)
{
    const std::array<float, 4> delta = ovtr::normalizeQuaternion(
        ovtr::multiplyQuaternion(ovtr::conjugateQuaternion(left), right)
    );
    const double w = std::clamp(std::fabs(static_cast<double>(delta[3])), 0.0, 1.0);
    return std::acos(w) * 114.59155902616465;
}

void writeVec(std::ofstream& out, const Vec3 v)
{
    out << v.x << ',' << v.y << ',' << v.z;
}

void writeQuat(std::ofstream& out, const std::array<float, 4>& q)
{
    out << q[0] << ',' << q[1] << ',' << q[2] << ',' << q[3];
}

void writeRow(std::ofstream& out, const MappingActor& actor, const char* phase, const int frame, const int slot)
{
    const std::size_t index = static_cast<std::size_t>(slot);
    const MappingVirtualTarget& target = actor.liveVirtualTargets[index];
    const MappingTransform expected = actor.calibration.trackerToTarget[index];
    const MappingTransform observed = target.valid
        ? composeMappingTransforms(inverseMappingTransform(target.trackerTransform), target.transform)
        : MappingTransform{};
    const float positionError = target.valid ? distanceMappingVec3(expected.position, observed.position) : -1.0f;
    const double rotationError = target.valid
        ? quaternionAngleDegrees(expected.rotation, observed.rotation)
        : -1.0;
    out << frame << ',' << phase << ',' << actor.id << ',' << slot << ','
        << roleName(mappingRoleForSlot(slot)) << ',' << target.valid << ','
        << positionError << ',' << rotationError << ',';
    writeVec(out, expected.position); out << ','; writeVec(out, observed.position); out << ',';
    writeQuat(out, expected.rotation); out << ','; writeQuat(out, observed.rotation); out << ',';
    writeVec(out, target.trackerTransform.position); out << ','; writeVec(out, target.transform.position);
    out << '\n';
}

void writeRows(std::ofstream& out, const MappingActor& actor, const char* phase, const int frame)
{
    for (int slot = 0; slot < kMappingSlotCount; ++slot) {
        writeRow(out, actor, phase, frame, slot);
    }
}

bool openAppend(std::ofstream& out, const std::filesystem::path& path)
{
    out.open(path, std::ios::app);
    return static_cast<bool>(out);
}

} // namespace

bool startMappingCalibrationTargetOffsetDebugLog(const MappingActor& actor, const std::filesystem::path& path)
{
    g_targetOffsetDebug = TargetOffsetDebugState{path, actor.id, 0, true};
    std::ofstream out(path);
    if (!out) {
        g_targetOffsetDebug.active = false;
        return false;
    }
    out << std::fixed << std::setprecision(6);
    out << "frame,phase,actor_id,slot,role,target_valid,offset_pos_error_m,offset_rot_error_deg,"
        << "expected_px,expected_py,expected_pz,observed_px,observed_py,observed_pz,"
        << "expected_qx,expected_qy,expected_qz,expected_qw,observed_qx,observed_qy,observed_qz,observed_qw,"
        << "tracker_x,tracker_y,tracker_z,target_x,target_y,target_z\n";
    writeRows(out, actor, "capture_rest", -1);
    return true;
}

void appendMappingCalibrationTargetOffsetDebugLog(const MappingActor& actor)
{
    if (!g_targetOffsetDebug.active || actor.id != g_targetOffsetDebug.actorId ||
        g_targetOffsetDebug.frame >= kPostSolveFrameLimit) {
        return;
    }
    std::ofstream out;
    if (!openAppend(out, g_targetOffsetDebug.path)) {
        g_targetOffsetDebug.active = false;
        return;
    }
    out << std::fixed << std::setprecision(6);
    writeRows(out, actor, "post_solve", g_targetOffsetDebug.frame++);
}

} // namespace ovtr::win32
