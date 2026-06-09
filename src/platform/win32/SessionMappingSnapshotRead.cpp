#include "platform/win32/SessionMappingSnapshot.h"

#include "data/SkeletalSyntheticPose.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/ConfigTextInternal.h"
#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/Win32String.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace ovtr::win32 {
namespace {

using ValueMap = std::unordered_map<std::string, std::string>;

bool parseValues(const std::filesystem::path& path, ValueMap& values, std::string& error)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "could not open mapping snapshot: " + path.string();
        return false;
    }
    std::string line;
    while (std::getline(input, line)) {
        line = trimAscii(line);
        if (line.empty() || line.front() == '#') {
            continue;
        }
        detail::ConfigAssignment assignment;
        if (!detail::parseConfigAssignmentLine(line, assignment)) {
            error = "invalid mapping snapshot line: " + line;
            return false;
        }
        values[assignment.key] = assignment.value;
    }
    return true;
}

void assignLegacyFingerRuntimeIndex(std::array<std::uint32_t, kMappingFingerSourceCount>& runtimeIndices, const std::uint32_t runtimeIndex) noexcept
{
    ovtr::SkeletalHandSide side = ovtr::SkeletalHandSide::Left;
    std::uint32_t boneIndex = 0;
    if (!ovtr::decodeSkeletalBoneRuntimeIndex(runtimeIndex, side, boneIndex)) {
        return;
    }
    runtimeIndices[side == ovtr::SkeletalHandSide::Left ? 0U : 1U] = runtimeIndex;
}

bool parseFloat(const ValueMap& values, const std::string& key, float& out, std::string& error)
{
    const auto found = values.find(key);
    if (found == values.end()) {
        error = "missing mapping snapshot field: " + key;
        return false;
    }
    char* end = nullptr;
    const float parsed = std::strtof(found->second.c_str(), &end);
    if (end == found->second.c_str() || *end != '\0' || !std::isfinite(parsed)) {
        error = "invalid mapping snapshot float: " + key;
        return false;
    }
    out = parsed;
    return true;
}

void parseOptionalFloat(const ValueMap& values, const std::string& key, float& out) noexcept
{
    const auto found = values.find(key);
    if (found == values.end()) {
        return;
    }
    char* end = nullptr;
    const float parsed = std::strtof(found->second.c_str(), &end);
    if (end != found->second.c_str() && *end == '\0' && std::isfinite(parsed)) {
        out = parsed;
    }
}

void parseOptionalBool(const ValueMap& values, const std::string& key, bool& out) noexcept
{
    const auto found = values.find(key);
    if (found != values.end() && (found->second == "1" || found->second == "true" || found->second == "enabled")) { out = true; }
    else if (found != values.end() && (found->second == "0" || found->second == "false" || found->second == "disabled")) { out = false; }
}

std::uint32_t parseUInt(const ValueMap& values, const std::string& key, const std::uint32_t fallback)
{
    const auto found = values.find(key);
    if (found == values.end()) {
        return fallback;
    }
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(found->second.c_str(), &end, 10);
    return *end == '\0' ? static_cast<std::uint32_t>(parsed) : fallback;
}

RgbColor parseOptionalColor(const ValueMap& values, const std::string& prefix, const RgbColor fallback)
{
    return RgbColor{
        static_cast<int>(std::min<std::uint32_t>(255, parseUInt(values, prefix + "color_r", fallback.r))),
        static_cast<int>(std::min<std::uint32_t>(255, parseUInt(values, prefix + "color_g", fallback.g))),
        static_cast<int>(std::min<std::uint32_t>(255, parseUInt(values, prefix + "color_b", fallback.b)))
    };
}

std::uint32_t runtimeIndexForSerial(
    const ovtr::RecordingSession& session,
    const std::string& serial
) noexcept
{
    if (serial.empty()) {
        return kNoSelectedRuntimeIndex;
    }
    for (const DeviceDescriptor& device : session.devices) {
        if (device.serial == serial) {
            return device.runtimeIndex;
        }
    }
    return kNoSelectedRuntimeIndex;
}

bool readProfile(const ValueMap& values, const std::string& prefix, BodyProfile& profile, std::string& error)
{
    const auto name = values.find(prefix + "name");
    if (name == values.end() || trimAscii(name->second).empty()) {
        error = "missing mapping snapshot field: " + prefix + "name";
        return false;
    }
    profile.name = widen(name->second);
    const auto& definitions = profileMeasurementDefinitions();
    for (int i = 0; i < kProfileMeasurementCount; ++i) {
        const auto index = static_cast<std::size_t>(i);
        if (!parseFloat(values, prefix + definitions[index].key, profile.measurements[index], error)) {
            return false;
        }
    }
    return true;
}

bool readTransform(const ValueMap& values, const std::string& prefix, MappingTransform& transform, std::string& error)
{
    return parseFloat(values, prefix + "_pos_x", transform.position.x, error) &&
        parseFloat(values, prefix + "_pos_y", transform.position.y, error) &&
        parseFloat(values, prefix + "_pos_z", transform.position.z, error) &&
        parseFloat(values, prefix + "_rot_x", transform.rotation[0], error) &&
        parseFloat(values, prefix + "_rot_y", transform.rotation[1], error) &&
        parseFloat(values, prefix + "_rot_z", transform.rotation[2], error) &&
        parseFloat(values, prefix + "_rot_w", transform.rotation[3], error);
}

int parentFootSlotFor(const int slot) noexcept
{
    const MappingTrackerRole role = mappingRoleForSlot(slot);
    if (role == MappingTrackerRole::LeftLeg) {
        return mappingSlotForRole(MappingTrackerRole::LeftFoot);
    }
    if (role == MappingTrackerRole::RightLeg) {
        return mappingSlotForRole(MappingTrackerRole::RightFoot);
    }
    return -1;
}

MappingVirtualTargetBinding inferredBinding(
    const std::array<std::uint32_t, kMappingSlotCount>& runtimeIndices,
    const int slot
) noexcept {
    const auto index = static_cast<std::size_t>(slot);
    if (runtimeIndices[index] != kNoSelectedRuntimeIndex) {
        return MappingVirtualTargetBinding{MappingVirtualTargetSource::DirectTracker, -1};
    }
    const int parentSlot = parentFootSlotFor(slot);
    if (parentSlot >= 0 &&
        runtimeIndices[static_cast<std::size_t>(parentSlot)] != kNoSelectedRuntimeIndex) {
        return MappingVirtualTargetBinding{MappingVirtualTargetSource::ParentedTracker, parentSlot};
    }
    return MappingVirtualTargetBinding{MappingVirtualTargetSource::RestFallback, -1};
}

MappingVirtualTargetBinding readBinding(
    const ValueMap& values,
    const std::string& prefix,
    const std::array<std::uint32_t, kMappingSlotCount>& runtimeIndices,
    const int slot
) noexcept {
    const auto found = values.find(prefix + "_target_source");
    if (found == values.end()) {
        return inferredBinding(runtimeIndices, slot);
    }
    if (found->second == "rest") {
        return MappingVirtualTargetBinding{MappingVirtualTargetSource::RestFallback, -1};
    }
    if (found->second == "parented") {
        const int parentSlot = static_cast<int>(
            parseUInt(values, prefix + "_target_parent_slot", static_cast<std::uint32_t>(kMappingSlotCount))
        );
        return MappingVirtualTargetBinding{MappingVirtualTargetSource::ParentedTracker, parentSlot};
    }
    if (found->second == "direct") {
        return MappingVirtualTargetBinding{MappingVirtualTargetSource::DirectTracker, -1};
    }
    return inferredBinding(runtimeIndices, slot);
}

bool readActor(
    const ValueMap& values,
    const ovtr::RecordingSession& session,
    const std::size_t actorIndex,
    MappingActor& actor,
    std::string& error
)
{
    const std::string prefix = "actor_" + std::to_string(actorIndex) + "_";
    actor.id = parseUInt(values, prefix + "id", static_cast<std::uint32_t>(actorIndex + 1));
    actor.calibrated = true;
    if (!readProfile(values, prefix, actor.profile, error)) {
        return false;
    }
    const auto actorName = values.find(prefix + "actor_name");
    actor.name = actorName == values.end() ? actor.profile.name : widen(actorName->second);
    actor.skeletonColor = parseOptionalColor(values, prefix, actor.skeletonColor);
    actor.mappingFingerRuntimeIndices[0] = parseUInt(values, prefix + "left_finger_runtime_index", kNoSelectedRuntimeIndex);
    actor.mappingFingerRuntimeIndices[1] = parseUInt(values, prefix + "right_finger_runtime_index", kNoSelectedRuntimeIndex);
    assignLegacyFingerRuntimeIndex(actor.mappingFingerRuntimeIndices, parseUInt(values, prefix + "finger_runtime_index", kNoSelectedRuntimeIndex));
    actor.liveJoints = buildProfileSkeletonJoints(actor.profile);
    actor.calibration.armSoftIkStrength = kDefaultMappingArmSoftIkStrength;
    actor.calibration.legSoftIkStrength = kDefaultMappingLegSoftIkStrength;
    parseOptionalFloat(values, prefix + "arm_soft_ik", actor.calibration.armSoftIkStrength);
    parseOptionalFloat(values, prefix + "leg_soft_ik", actor.calibration.legSoftIkStrength);
    parseOptionalBool(values, prefix + "pin_hand", actor.calibration.pinHandTargets);
    parseOptionalBool(values, prefix + "pin_foot", actor.calibration.pinFootTargets);
    for (int slot = 0; slot < kMappingSlotCount; ++slot) {
        const auto index = static_cast<std::size_t>(slot);
        const std::string slotPrefix = prefix + "slot_" + std::to_string(slot);
        const auto serial = values.find(slotPrefix + "_serial");
        actor.calibration.runtimeIndices[index] =
            runtimeIndexForSerial(session, serial == values.end() ? "" : serial->second);
        actor.mappingDeviceRuntimeIndices[index] = actor.calibration.runtimeIndices[index];
        if (!readTransform(values, slotPrefix, actor.calibration.trackerToTarget[index], error)) {
            return false;
        }
        actor.calibration.targetBindings[index] = readBinding(
            values,
            slotPrefix,
            actor.calibration.runtimeIndices,
            slot
        );
    }
    return true;
}

} // namespace

bool loadSessionMappingSnapshot(
    const std::filesystem::path& sessionFolder,
    const ovtr::RecordingSession& session,
    SessionMappingSnapshot& outSnapshot,
    std::string& error
)
{
    ValueMap values;
    if (!parseValues(sessionMappingSnapshotPath(sessionFolder), values, error)) {
        return false;
    }

    SessionMappingSnapshot snapshot;
    if (!readProfile(values, "profile_", snapshot.profile, error)) {
        return false;
    }
    snapshot.mappingSkeletonColor = parseOptionalColor(values, "mapping_", snapshot.mappingSkeletonColor);
    for (int slot = 0; slot < kMappingSlotCount; ++slot) {
        const auto key = "mapping_slot_" + std::to_string(slot) + "_serial";
        const auto found = values.find(key);
        snapshot.mappingDeviceRuntimeIndices[static_cast<std::size_t>(slot)] =
            runtimeIndexForSerial(session, found == values.end() ? "" : found->second);
    }
    snapshot.mappingFingerRuntimeIndices[0] = parseUInt(values, "mapping_left_finger_runtime_index", kNoSelectedRuntimeIndex);
    snapshot.mappingFingerRuntimeIndices[1] = parseUInt(values, "mapping_right_finger_runtime_index", kNoSelectedRuntimeIndex);
    assignLegacyFingerRuntimeIndex(snapshot.mappingFingerRuntimeIndices, parseUInt(values, "mapping_finger_runtime_index", kNoSelectedRuntimeIndex));

    const std::uint32_t count = parseUInt(values, "actor_count", 0);
    snapshot.selectedActorId = parseUInt(values, "selected_actor_id", 0);
    for (std::uint32_t i = 0; i < count; ++i) {
        MappingActor actor;
        if (!readActor(values, session, i, actor, error)) {
            return false;
        }
        snapshot.actors.push_back(std::move(actor));
    }
    outSnapshot = std::move(snapshot);
    return true;
}

} // namespace ovtr::win32
