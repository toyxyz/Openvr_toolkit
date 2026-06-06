#pragma once

#include "data/SessionTypes.h"
#include "platform/win32/AppProfileState.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct AppLoadedSessionState;

struct SessionMappingSnapshot {
    BodyProfile profile;
    RgbColor mappingSkeletonColor{255, 255, 255};
    std::array<std::uint32_t, kMappingSlotCount> mappingDeviceRuntimeIndices =
        defaultMappingDeviceRuntimeIndices();
    std::array<std::uint32_t, kMappingFingerSourceCount> mappingFingerRuntimeIndices =
        defaultMappingFingerRuntimeIndices();
    std::vector<MappingActor> actors;
    std::uint32_t selectedActorId = 0;
};

std::filesystem::path sessionMappingSnapshotPath(const std::filesystem::path& sessionFolder);

void backupLiveMappingForLoadedSession(AppLoadedSessionState& loadedState, const AppProfileState& profileState);
void restoreLiveMappingAfterLoadedSession(AppLoadedSessionState& loadedState, AppProfileState& profileState);
void applySessionMappingSnapshotToLiveMapping(AppProfileState& profileState, SessionMappingSnapshot snapshot);

bool saveSessionMappingSnapshot(
    const AppProfileState& profileState,
    const std::vector<DeviceDescriptor>& devices,
    const std::filesystem::path& sessionFolder,
    std::string& error
);

bool loadSessionMappingSnapshot(
    const std::filesystem::path& sessionFolder,
    const ovtr::RecordingSession& session,
    SessionMappingSnapshot& outSnapshot,
    std::string& error
);

} // namespace ovtr::win32
