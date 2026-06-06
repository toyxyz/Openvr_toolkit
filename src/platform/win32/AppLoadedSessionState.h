#pragma once

#include "data/SessionTypes.h"
#include "platform/win32/AppProfileState.h"
#include "recording/BinarySessionReader.h"
#include "recording/SessionManifest.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct AppLoadedSessionState {
    bool loadedSessionActive = false;
    bool loadedSessionPlaying = false;
    bool loadedSessionTimelineDragging = false;
    bool loadedSessionOriginWasEnabled = false;
    std::wstring loadedSessionPreviousSessionName;
    std::array<float, 3> loadedSessionOriginOffset{};
    std::array<float, 3> loadedSessionOriginRotationDegrees{};
    double loadedSessionPlaybackSeconds = 0.0;
    double loadedSessionDurationSeconds = 0.0;
    std::filesystem::path loadedSessionFolder;
    std::string loadedSessionStatusMessage;
    RecordingSession loadedSession;
    SessionManifestStats loadedSessionStats;
    BinarySessionReader loadedSessionReader;
    bool loadedSessionMappingSnapshotLoaded = false;
    bool loadedSessionLastSampledFrameValid = false;
    std::uint64_t loadedSessionLastSampledFrameIndex = 0;
    bool loadedSessionLiveMappingBackupValid = false;
    BodyProfile loadedSessionLiveProfile;
    std::wstring loadedSessionLiveMappingActorName;
    RgbColor loadedSessionLiveMappingSkeletonColor{255, 255, 255};
    bool loadedSessionLiveMappingSkeletonColorCustomized = false;
    std::array<std::uint32_t, kMappingSlotCount> loadedSessionLiveMappingDeviceRuntimeIndices =
        defaultMappingDeviceRuntimeIndices();
    std::array<std::uint32_t, kMappingFingerSourceCount> loadedSessionLiveMappingFingerRuntimeIndices =
        defaultMappingFingerRuntimeIndices();
    std::vector<MappingActor> loadedSessionLiveMappingActors;
    std::uint32_t loadedSessionLiveNextMappingActorId = 1;
    std::uint32_t loadedSessionLiveSelectedMappingActorId = 0;
    float loadedSessionLiveArmSoftIkStrength = kDefaultMappingArmSoftIkStrength;
    float loadedSessionLiveLegSoftIkStrength = kDefaultMappingLegSoftIkStrength;
    std::chrono::steady_clock::time_point loadedSessionLastUpdate{};
};

} // namespace ovtr::win32
