#include "platform/win32/SessionMappingSnapshot.h"

#include "platform/win32/ProfileStore.h"
#include "platform/win32/Win32String.h"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace ovtr::win32 {
namespace {

std::wstring serialForRuntimeIndex(
    const std::vector<DeviceDescriptor>& devices,
    const std::uint32_t runtimeIndex
)
{
    for (const DeviceDescriptor& device : devices) {
        if (device.runtimeIndex == runtimeIndex) {
            return widen(device.serial);
        }
    }
    return {};
}

void writeProfile(std::ostream& out, const char* prefix, const BodyProfile& profile)
{
    out << prefix << "name=" << narrow(profile.name) << "\n";
    const auto& definitions = profileMeasurementDefinitions();
    for (int i = 0; i < kProfileMeasurementCount; ++i) {
        const auto index = static_cast<std::size_t>(i);
        out << prefix << definitions[index].key << "=" << profile.measurements[index] << "\n";
    }
}

void writeColor(std::ostream& out, const char* prefix, const RgbColor color)
{
    out << prefix << "color_r=" << color.r << "\n";
    out << prefix << "color_g=" << color.g << "\n";
    out << prefix << "color_b=" << color.b << "\n";
}

std::size_t calibratedActorCount(const AppProfileState& profileState) noexcept
{
    std::size_t count = 0;
    for (const MappingActor& actor : profileState.mappingActors) {
        if (actor.calibrated) {
            ++count;
        }
    }
    return count;
}

void writeTransform(std::ostream& out, const std::string& prefix, const MappingTransform& transform)
{
    out << prefix << "_pos_x=" << transform.position.x << "\n";
    out << prefix << "_pos_y=" << transform.position.y << "\n";
    out << prefix << "_pos_z=" << transform.position.z << "\n";
    out << prefix << "_rot_x=" << transform.rotation[0] << "\n";
    out << prefix << "_rot_y=" << transform.rotation[1] << "\n";
    out << prefix << "_rot_z=" << transform.rotation[2] << "\n";
    out << prefix << "_rot_w=" << transform.rotation[3] << "\n";
}

const char* targetSourceName(const MappingVirtualTargetSource source) noexcept
{
    if (source == MappingVirtualTargetSource::RestFallback) {
        return "rest";
    }
    if (source == MappingVirtualTargetSource::ParentedTracker) {
        return "parented";
    }
    return "direct";
}

void writeActor(
    std::ostream& out,
    const MappingActor& actor,
    const std::vector<DeviceDescriptor>& devices,
    const std::size_t actorIndex
)
{
    const std::string prefix = "actor_" + std::to_string(actorIndex) + "_";
    out << prefix << "id=" << actor.id << "\n";
    writeProfile(out, prefix.c_str(), actor.profile);
    writeColor(out, prefix.c_str(), actor.skeletonColor);
    out << prefix << "arm_soft_ik=" << actor.calibration.armSoftIkStrength << "\n";
    out << prefix << "leg_soft_ik=" << actor.calibration.legSoftIkStrength << "\n";
    for (int slot = 0; slot < kMappingSlotCount; ++slot) {
        const auto index = static_cast<std::size_t>(slot);
        const std::string slotPrefix = prefix + "slot_" + std::to_string(slot);
        out << slotPrefix << "_serial="
            << narrow(serialForRuntimeIndex(devices, actor.calibration.runtimeIndices[index])) << "\n";
        out << slotPrefix << "_target_source="
            << targetSourceName(actor.calibration.targetBindings[index].source) << "\n";
        out << slotPrefix << "_target_parent_slot="
            << actor.calibration.targetBindings[index].parentSlot << "\n";
        writeTransform(out, slotPrefix, actor.calibration.trackerToTarget[index]);
    }
}

} // namespace

std::filesystem::path sessionMappingSnapshotPath(const std::filesystem::path& sessionFolder)
{
    return sessionFolder / "mapping_snapshot.cfg";
}

bool saveSessionMappingSnapshot(
    const AppProfileState& profileState,
    const std::vector<DeviceDescriptor>& devices,
    const std::filesystem::path& sessionFolder,
    std::string& error
)
{
    const std::filesystem::path path = sessionMappingSnapshotPath(sessionFolder);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        error = "could not open mapping snapshot for writing: " + path.string();
        return false;
    }

    out << "# toyxyz_openvr_toolkit session mapping snapshot v1\n";
    out << "version=1\n";
    out << std::setprecision(9);
    writeProfile(out, "profile_", profileState.profile);
    writeColor(out, "mapping_", profileState.mappingSkeletonColor);
    for (int slot = 0; slot < kMappingSlotCount; ++slot) {
        const auto runtimeIndex = profileState.mappingDeviceRuntimeIndices[static_cast<std::size_t>(slot)];
        out << "mapping_slot_" << slot << "_serial="
            << narrow(serialForRuntimeIndex(devices, runtimeIndex)) << "\n";
    }

    out << "selected_actor_id=" << profileState.selectedMappingActorId << "\n";
    out << "actor_count=" << calibratedActorCount(profileState) << "\n";
    std::size_t actorIndex = 0;
    for (const MappingActor& actor : profileState.mappingActors) {
        if (actor.calibrated) {
            writeActor(out, actor, devices, actorIndex++);
        }
    }
    if (!out) {
        error = "failed while writing mapping snapshot: " + path.string();
        return false;
    }
    return true;
}

} // namespace ovtr::win32
