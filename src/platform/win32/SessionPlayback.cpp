#include "platform/win32/SessionPlayback.h"
#include "data/SkeletalSyntheticPose.h"
#include "data/VmcSyntheticPose.h"
#include "platform/win32/AppLoadedSessionState.h"
#include "platform/win32/AppOriginState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/AppState.h"
#include "platform/win32/SessionMappingSnapshot.h"
#include "platform/win32/SessionPlaybackFrameIndex.h"
#include "recording/SessionManifest.h"
#include <algorithm>
#include <utility>
namespace ovtr::win32 {
namespace {

std::filesystem::path sessionFilePath(const std::filesystem::path& folder, const std::filesystem::path& manifestPath, const char* fallbackName)
{
    if (!manifestPath.empty() && manifestPath.is_absolute()) {
        return manifestPath;
    }
    if (!manifestPath.empty()) {
        return folder / manifestPath.filename();
    }
    return folder / fallbackName;
}
DeviceDescriptor fallbackDeviceForPose(const PoseSample& pose)
{
    DeviceDescriptor device;
    device.id = pose.deviceId;
    device.runtimeIndex = pose.runtimeIndex;
    device.deviceClass = DeviceClass::Other;
    device.serial = "runtime_" + std::to_string(pose.runtimeIndex);
    device.displayName = device.serial;
    device.recordEnabled = (pose.flags & PoseFlagRecordEnabled) != 0;
    return device;
}
void synthesizeMissingDevices(RecordingSession& session, const FrameSample& firstFrame)
{
    if (!session.devices.empty()) {
        return;
    }
    for (const PoseSample& pose : firstFrame.poses) {
        if (!isSkeletalBoneRuntimeIndex(pose.runtimeIndex) && !isVmcFingerRuntimeIndex(pose.runtimeIndex)) {
            session.devices.push_back(fallbackDeviceForPose(pose));
        }
    }
}
double sessionDurationSeconds(const SessionManifestStats& stats, const RecordingSession& session, const FrameSample& lastFrame, const std::uint64_t frameCount) noexcept
{
    if (stats.durationSeconds > 0.0) {
        return stats.durationSeconds;
    }
    if (lastFrame.timeSeconds > 0.0) {
        return lastFrame.timeSeconds;
    }
    if (frameCount > 1 && session.targetSampleRate > 0.0) {
        return static_cast<double>(frameCount - 1) / session.targetSampleRate;
    }
    return 0.0;
}
} // namespace
bool openLoadedSession(AppWindowState& state, const std::filesystem::path& folder, std::string& outError)
{
    outError.clear();
    RecordingSession session;
    SessionManifestStats stats;
    if (!readManifestJson(folder / "manifest.json", session, stats, outError)) {
        return false;
    }

    session.framesPath = sessionFilePath(folder, session.framesPath, "frames.bin");
    session.frameIndexPath = sessionFilePath(folder, session.frameIndexPath, "frame_index.bin");

    BinarySessionReader reader;
    if (!reader.open(session.framesPath, session.frameIndexPath)) {
        outError = reader.lastError();
        return false;
    }
    if (reader.frameCount() == 0) {
        outError = "session has no frames";
        return false;
    }

    FrameSample firstFrame;
    FrameSample lastFrame;
    if (!reader.readFrame(0, firstFrame) || !reader.readFrame(reader.frameCount() - 1, lastFrame)) {
        outError = reader.lastError();
        return false;
    }
    synthesizeMissingDevices(session, firstFrame);

    SessionMappingSnapshot mappingSnapshot;
    bool hasMappingSnapshot = false;
    std::error_code snapshotExistsError;
    if (std::filesystem::exists(sessionMappingSnapshotPath(folder), snapshotExistsError)) {
        if (!loadSessionMappingSnapshot(folder, session, mappingSnapshot, outError)) {
            outError = "failed to load session mapping snapshot: " + outError;
            return false;
        }
        hasMappingSnapshot = true;
    } else if (snapshotExistsError) {
        outError = "failed to inspect session mapping snapshot: " + snapshotExistsError.message();
        return false;
    }

    closeLoadedSession(state);
    state.loadedSessionPreviousSessionName = state.sessionName;
    if (hasMappingSnapshot) {
        backupLiveMappingForLoadedSession(
            static_cast<AppLoadedSessionState&>(state),
            static_cast<const AppProfileState&>(state)
        );
        applySessionMappingSnapshotToLiveMapping(
            static_cast<AppProfileState&>(state),
            std::move(mappingSnapshot)
        );
    }
    state.loadedSessionReader = std::move(reader);
    state.loadedSession = std::move(session);
    state.loadedSessionStats = stats;
    state.loadedSessionFolder = folder;
    state.sessionName = folder.filename().wstring();
    state.loadedSessionDurationSeconds = sessionDurationSeconds(stats, state.loadedSession, lastFrame, reader.frameCount());
    state.loadedSessionOriginWasEnabled = state.originEnabled;
    state.loadedSessionOriginOffset = state.originOffset;
    state.loadedSessionOriginRotationDegrees = state.originRotationDegrees;
    state.originEnabled = false;
    state.originOffset = {};
    state.originRotationDegrees = {};
    state.devices = state.loadedSession.devices;
    state.poses.timestampNs = firstFrame.timestampNs;
    state.poses.poses = firstFrame.poses;
    state.loadedSessionPlaybackSeconds = 0.0;
    state.loadedSessionLastUpdate = std::chrono::steady_clock::now();
    state.loadedSessionPlaying = false;
    state.loadedSessionTimelineDragging = false;
    state.loadedSessionMappingSnapshotLoaded = hasMappingSnapshot;
    state.loadedSessionLastSampledFrameValid = false;
    state.loadedSessionLastSampledFrameIndex = 0;
    state.loadedSessionActive = true;
    return true;
}

bool closeLoadedSession(AppWindowState& state)
{
    const bool wasActive = state.loadedSessionActive;
    state.loadedSessionReader.close();
    state.loadedSessionActive = false;
    state.loadedSessionPlaying = false;
    state.loadedSessionTimelineDragging = false;
    state.loadedSessionPlaybackSeconds = 0.0;
    state.loadedSessionDurationSeconds = 0.0;
    state.loadedSessionFolder.clear();
    state.loadedSessionStatusMessage.clear();
    state.loadedSession = RecordingSession{};
    state.loadedSessionStats = SessionManifestStats{};
    state.loadedSessionMappingSnapshotLoaded = false;
    state.loadedSessionLastSampledFrameValid = false;
    state.loadedSessionLastSampledFrameIndex = 0;
    state.loadedSessionLastUpdate = {};
    if (wasActive) {
        state.sessionName = state.loadedSessionPreviousSessionName;
        state.originEnabled = state.loadedSessionOriginWasEnabled;
        state.originOffset = state.loadedSessionOriginOffset;
        state.originRotationDegrees = state.loadedSessionOriginRotationDegrees;
        restoreLiveMappingAfterLoadedSession(
            static_cast<AppLoadedSessionState&>(state),
            static_cast<AppProfileState&>(state)
        );
    }
    state.loadedSessionPreviousSessionName.clear();
    state.loadedSessionLiveProfile = BodyProfile{};
    state.loadedSessionLiveMappingDeviceRuntimeIndices = defaultMappingDeviceRuntimeIndices();
    state.loadedSessionLiveMappingActorName.clear();
    state.loadedSessionLiveMappingFingerRuntimeIndices = defaultMappingFingerRuntimeIndices();
    state.loadedSessionLiveMappingActors.clear();
    state.loadedSessionLiveNextMappingActorId = 1;
    state.loadedSessionLiveSelectedMappingActorId = 0;
    state.loadedSessionLiveArmSoftIkStrength = kDefaultMappingArmSoftIkStrength;
    state.loadedSessionLiveLegSoftIkStrength = kDefaultMappingLegSoftIkStrength;
    return wasActive;
}

double loadedSessionDurationSeconds(const AppLoadedSessionState& state) noexcept
{
    return state.loadedSessionActive && state.loadedSessionDurationSeconds > 0.0
        ? state.loadedSessionDurationSeconds
        : 0.0;
}

int loadedSessionTotalFrames(const AppLoadedSessionState& state) noexcept
{
    return state.loadedSessionActive
        ? static_cast<int>(state.loadedSessionReader.frameCount())
        : 0;
}

int loadedSessionCurrentFrame(const AppLoadedSessionState& state) noexcept
{
    if (!state.loadedSessionActive) {
        return 0;
    }
    return static_cast<int>(loadedSessionFrameIndexForPlayback(state)) + 1;
}

double loadedSessionPlaybackTime(const AppLoadedSessionState& state) noexcept
{
    return state.loadedSessionActive
        ? std::clamp(state.loadedSessionPlaybackSeconds, 0.0, loadedSessionDurationSeconds(state))
        : 0.0;
}

void setLoadedSessionPlaybackSeconds(AppLoadedSessionState& state, const double playbackSeconds, const std::chrono::steady_clock::time_point now) noexcept
{
    state.loadedSessionPlaybackSeconds = std::clamp(playbackSeconds, 0.0, loadedSessionDurationSeconds(state));
    state.loadedSessionLastUpdate = now;
    state.loadedSessionLastSampledFrameValid = false;
}

void setLoadedSessionPlaybackSeconds(AppLoadedSessionState& state, const double playbackSeconds) noexcept
{
    setLoadedSessionPlaybackSeconds(state, playbackSeconds, std::chrono::steady_clock::now());
}

void updateLoadedSessionPlayback(AppLoadedSessionState& state, const std::chrono::steady_clock::time_point now) noexcept
{
    if (!state.loadedSessionActive) {
        return;
    }
    if (state.loadedSessionLastUpdate == std::chrono::steady_clock::time_point{}) {
        state.loadedSessionLastUpdate = now;
    }
    if (!state.loadedSessionPlaying || state.loadedSessionTimelineDragging) {
        state.loadedSessionLastUpdate = now;
        return;
    }
    const double elapsed = std::chrono::duration<double>(now - state.loadedSessionLastUpdate).count();
    state.loadedSessionLastUpdate = now;
    state.loadedSessionPlaybackSeconds += elapsed > 0.0 ? elapsed : 0.0;
    if (state.loadedSessionPlaybackSeconds >= loadedSessionDurationSeconds(state)) {
        state.loadedSessionPlaybackSeconds = loadedSessionDurationSeconds(state);
        state.loadedSessionPlaying = false;
    }
}

void updateLoadedSessionPlayback(AppLoadedSessionState& state) noexcept
{
    updateLoadedSessionPlayback(state, std::chrono::steady_clock::now());
}

bool seekLoadedSessionFromTimeline(AppLoadedSessionState& state, const RECT& timelineRect, const POINT point) noexcept
{
    if (!state.loadedSessionActive || timelineRect.right <= timelineRect.left) {
        return false;
    }
    const int clampedX = std::clamp(point.x, timelineRect.left, timelineRect.right);
    const double factor = static_cast<double>(clampedX - timelineRect.left) / static_cast<double>(timelineRect.right - timelineRect.left);
    setLoadedSessionPlaybackSeconds(state, factor * loadedSessionDurationSeconds(state));
    return true;
}

SessionPlaybackToggleResult toggleLoadedSessionPlayback(AppLoadedSessionState& state, const std::chrono::steady_clock::time_point now) noexcept
{
    if (!state.loadedSessionActive) {
        return SessionPlaybackToggleResult::Ignored;
    }
    if (!state.loadedSessionPlaying && state.loadedSessionPlaybackSeconds >= loadedSessionDurationSeconds(state)) {
        setLoadedSessionPlaybackSeconds(state, 0.0, now);
    }
    state.loadedSessionPlaying = !state.loadedSessionPlaying;
    state.loadedSessionLastUpdate = now;
    return state.loadedSessionPlaying ? SessionPlaybackToggleResult::Started : SessionPlaybackToggleResult::Paused;
}

SessionPlaybackToggleResult toggleLoadedSessionPlayback(AppLoadedSessionState& state) noexcept
{
    return toggleLoadedSessionPlayback(state, std::chrono::steady_clock::now());
}

} // namespace ovtr::win32
