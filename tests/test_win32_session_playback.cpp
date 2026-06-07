#include "TestCases.h"
#include "TestSupport.h"

#include "platform/win32/AppState.h"
#include "platform/win32/MappingCalibrationCapture.h"
#include "platform/win32/MappingCalibrationSolve.h"
#include "platform/win32/MappingCalibrationTargets.h"
#include "platform/win32/SessionExportActions.h"
#include "platform/win32/SessionPlayback.h"
#include "recording/SessionManifest.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace ovtr::test {
namespace {

constexpr std::uint32_t kLoadedMappingRuntimeBase = 100;

float distanceBetween(const ovtr::win32::Vec3 a, const ovtr::win32::Vec3 b) noexcept
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

std::array<std::uint32_t, ovtr::win32::kMappingSlotCount> loadedMappingRuntimeIndices() noexcept
{
    std::array<std::uint32_t, ovtr::win32::kMappingSlotCount> indices{};
    for (int slot = 0; slot < ovtr::win32::kMappingSlotCount; ++slot) {
        indices[static_cast<std::size_t>(slot)] = kLoadedMappingRuntimeBase + static_cast<std::uint32_t>(slot);
    }
    return indices;
}

std::vector<ovtr::DeviceDescriptor> loadedMappingDevices()
{
    std::vector<ovtr::DeviceDescriptor> devices;
    const auto indices = loadedMappingRuntimeIndices();
    for (int slot = 0; slot < ovtr::win32::kMappingSlotCount; ++slot) {
        ovtr::DeviceDescriptor device;
        device.id = indices[static_cast<std::size_t>(slot)];
        device.runtimeIndex = device.id;
        device.deviceClass = ovtr::DeviceClass::GenericTracker;
        device.serial = "LHR-MAPPING-" + std::to_string(slot);
        device.displayName = "Mapping Tracker " + std::to_string(slot);
        device.recordEnabled = true;
        devices.push_back(device);
    }
    return devices;
}

ovtr::FrameSample loadedMappingFrame(
    const ovtr::win32::BodyProfile& profile,
    const std::uint64_t frameIndex,
    const float leftHandZOffset
)
{
    ovtr::FrameSample frame;
    frame.frameIndex = frameIndex;
    frame.timestampNs = frameIndex * 1'000'000'000ull;
    frame.timeSeconds = static_cast<double>(frameIndex);
    const auto indices = loadedMappingRuntimeIndices();
    const auto targets = ovtr::win32::mappingCalibrationRestTargets(profile);
    for (int slot = 0; slot < ovtr::win32::kMappingSlotCount; ++slot) {
        const std::size_t index = static_cast<std::size_t>(slot);
        ovtr::win32::Vec3 position = targets[index].position;
        if (ovtr::win32::mappingRoleForSlot(slot) == ovtr::win32::MappingTrackerRole::LeftHand) {
            position.z += leftHandZOffset;
        }

        ovtr::PoseSample pose;
        pose.deviceId = indices[index];
        pose.runtimeIndex = indices[index];
        pose.position = {position.x, position.y, position.z};
        pose.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        pose.flags = ovtr::PoseFlagDeviceConnected | ovtr::PoseFlagPoseValid | ovtr::PoseFlagRecordEnabled;
        frame.poses.push_back(pose);
    }
    return frame;
}

void verifyLoadedSessionDrivesMappingSkeleton(const std::filesystem::path& root)
{
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    require(std::filesystem::create_directories(root), "loaded mapping temp directory");

    ovtr::win32::AppWindowState state;
    const std::vector<ovtr::FrameSample> frames{
        loadedMappingFrame(state.profile, 0, 0.0f),
        loadedMappingFrame(state.profile, 1, 0.18f)
    };
    writeFrameSamples(root / "frames.bin", root / "frame_index.bin", frames, "loaded mapping");

    ovtr::RecordingSession session = makeTestSession(
        "loaded_mapping_session",
        "Loaded Mapping Session",
        "frames.bin",
        "frame_index.bin",
        loadedMappingDevices()
    );
    std::string error;
    require(
        ovtr::writeManifestJson(session, ovtr::SessionManifestStats{2, 1.0, 0, true}, root / "manifest.json", error),
        "loaded mapping manifest write failed: " + error
    );
    require(ovtr::win32::openLoadedSession(state, root, error), "loaded mapping open failed: " + error);

    state.mappingDeviceRuntimeIndices = loadedMappingRuntimeIndices();
    ovtr::win32::MappingActor actor;
    actor.profile = state.profile;
    const ovtr::win32::MappingCalibrationStatus status = ovtr::win32::captureMappingActorCalibration(
        actor,
        state.mappingDeviceRuntimeIndices,
        state.poses,
        state.originEnabled,
        state.originOffset,
        state.originRotationDegrees,
        state.mappingArmSoftIkStrength,
        state.mappingLegSoftIkStrength
    );
    require(status.success, "loaded mapping calibration should use session poses");
    const ovtr::win32::Vec3 firstWrist =
        actor.liveJoints[ovtr::win32::kProfileJointLeftForeArm].positionMeters;

    ovtr::win32::setLoadedSessionPlaybackSeconds(state, 1.0);
    require(ovtr::win32::sampleLoadedSessionFrame(state), "loaded mapping samples second frame");
    require(
        ovtr::win32::updateCalibratedMappingActorJoints(
            actor,
            state.poses,
            state.originEnabled,
            state.originOffset,
            state.originRotationDegrees
        ),
        "loaded mapping updates calibrated actor from session poses"
    );
    const ovtr::win32::Vec3 secondWrist =
        actor.liveJoints[ovtr::win32::kProfileJointLeftForeArm].positionMeters;
    require(distanceBetween(firstWrist, secondWrist) > 0.05f, "loaded timeline pose drives mapping skeleton");
    std::filesystem::remove_all(root, ignored);
}

void verifyLoadedSessionBuildsSkeletonExportClip(const std::filesystem::path& root)
{
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    require(std::filesystem::create_directories(root), "loaded export temp directory");

    ovtr::win32::AppWindowState state;
    const std::vector<ovtr::FrameSample> frames{
        loadedMappingFrame(state.profile, 0, 0.0f),
        loadedMappingFrame(state.profile, 1, 0.12f)
    };
    writeFrameSamples(root / "frames.bin", root / "frame_index.bin", frames, "loaded export");

    ovtr::RecordingSession session = makeTestSession(
        "loaded_export_session",
        "Loaded Export Session",
        "frames.bin",
        "frame_index.bin",
        loadedMappingDevices()
    );
    std::string error;
    require(
        ovtr::writeManifestJson(session, ovtr::SessionManifestStats{2, 1.0, 0, true}, root / "manifest.json", error),
        "loaded export manifest write failed: " + error
    );
    require(ovtr::win32::openLoadedSession(state, root, error), "loaded export open failed: " + error);

    state.mappingDeviceRuntimeIndices = loadedMappingRuntimeIndices();
    ovtr::win32::MappingActor actor;
    actor.id = 7;
    actor.name = L"Loaded Actor";
    actor.profile = state.profile;
    const ovtr::win32::MappingCalibrationStatus status = ovtr::win32::captureMappingActorCalibration(
        actor,
        state.mappingDeviceRuntimeIndices,
        state.poses,
        state.originEnabled,
        state.originOffset,
        state.originRotationDegrees,
        state.mappingArmSoftIkStrength,
        state.mappingLegSoftIkStrength
    );
    require(status.success, "loaded export calibration should use session poses");

    ovtr::win32::SkeletonRecordingClip clip;
    state.selectedMappingActorId = actor.id;
    state.mappingActors.push_back(actor);
    require(
        ovtr::win32::buildLoadedSessionSkeletonClip(state, clip, error),
        "loaded export skeleton clip failed: " + error
    );
    require(clip.actorId == actor.id, "loaded export clip uses calibrated actor");
    require(clip.actorName == L"Loaded Actor", "loaded export clip uses actor name");
    require(clip.frames.size() == 2, "loaded export clip frame count");
    require(clip.frames[1].timeSeconds > clip.frames[0].timeSeconds, "loaded export clip timeline");
    require(
        clip.frames[1].pose.timeSeconds == clip.frames[1].timeSeconds,
        "loaded export pose time matches frame time"
    );

    std::filesystem::remove_all(root, ignored);
}

} // namespace

void testWin32SessionPlayback()
{
    const std::filesystem::path root = std::filesystem::current_path() / ".tmp_ovtr_session_playback";
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    require(std::filesystem::create_directories(root), "session playback temp directory");

    const std::filesystem::path framesPath = root / "frames.bin";
    const std::filesystem::path indexPath = root / "frame_index.bin";
    writeTestFrames(framesPath, indexPath, 2, "session playback");

    ovtr::DeviceDescriptor tracker = makeTestTracker("LHR-LOAD");
    tracker.displayName = "Loaded Tracker";
    tracker.modelName = "VIVE Tracker Pro";
    tracker.renderModelName = "vr_tracker_vive_3_0";

    ovtr::RecordingSession session = makeTestSession(
        "loaded_session",
        "Loaded Session",
        "frames.bin",
        "frame_index.bin",
        {tracker}
    );
    session.targetSampleRate = 90.0;

    const ovtr::SessionManifestStats stats{2, 1.0, 0, true};
    std::string error;
    require(
        ovtr::writeManifestJson(session, stats, root / "manifest.json", error),
        "session playback manifest write failed: " + error
    );

    ovtr::win32::AppWindowState state;
    state.sessionName = L"before_load";
    state.originEnabled = true;
    state.originOffset = {1.0f, 2.0f, 3.0f};
    state.originRotationDegrees = {4.0f, 5.0f, 6.0f};

    require(ovtr::win32::openLoadedSession(state, root, error), "open loaded session failed: " + error);
    require(state.loadedSessionActive, "loaded session active");
    require(state.sessionName == root.filename().wstring(), "loaded session updates editable session name");
    require(!state.originEnabled, "loaded session disables live origin transform");
    require(state.devices.size() == 1, "loaded session restores manifest devices");
    require(state.devices[0].displayName == "Loaded Tracker", "loaded session device display name");
    require(state.devices[0].renderModelName == "vr_tracker_vive_3_0", "loaded session render model name");
    require(state.poses.poses.size() == 1, "loaded session samples first frame");
    require(state.poses.poses[0].position[0] == 1.0f, "loaded session first frame pose");

    ovtr::win32::setLoadedSessionPlaybackSeconds(state, 1.0);
    require(ovtr::win32::sampleLoadedSessionFrame(state), "loaded session samples seeked frame");
    require(ovtr::win32::loadedSessionCurrentFrame(state) == 2, "loaded session current frame");
    require(state.poses.poses[0].position[0] == 2.0f, "loaded session second frame pose");
    require(state.loadedSessionLastSampledFrameValid, "loaded session records sampled frame continuity");
    ovtr::win32::setLoadedSessionPlaybackSeconds(state, 0.0);
    require(!state.loadedSessionLastSampledFrameValid, "loaded session explicit seek resets continuity");

    require(ovtr::win32::closeLoadedSession(state), "close loaded session reports active state");
    require(!state.loadedSessionActive, "loaded session closes");
    require(state.sessionName == L"before_load", "loaded session close restores editable session name");
    require(state.originEnabled, "loaded session close restores origin enabled");
    require(state.originOffset[0] == 1.0f, "loaded session close restores origin offset");

    std::filesystem::remove_all(root, ignored);
    verifyLoadedSessionDrivesMappingSkeleton(std::filesystem::current_path() / ".tmp_ovtr_loaded_mapping");
    verifyLoadedSessionBuildsSkeletonExportClip(std::filesystem::current_path() / ".tmp_ovtr_loaded_export");
}

} // namespace ovtr::test
