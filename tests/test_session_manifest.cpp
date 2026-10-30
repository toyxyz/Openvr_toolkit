#include "TestCases.h"
#include "TestSupport.h"

#include "recording/SessionManifest.h"
#include "recording/SessionManifestRecovery.h"

#include <filesystem>
#include <string>

namespace ovtr::test {

void testManifestWriter()
{
    ovtr::RecordingSession session;
    session.sessionId = "test-session";
    session.sessionName = "Unit Test Session";
    session.createdAtUtc = "2026-05-25T00:00:00Z";
    session.appVersion = "0.1.0";
    session.framesPath = "frames.bin";
    session.frameIndexPath = "frame_index.bin";
    session.videoPath = "video.mp4";
    ovtr::DeviceDescriptor tracker;
    tracker.id = 42;
    tracker.runtimeIndex = 7;
    tracker.deviceClass = ovtr::DeviceClass::GenericTracker;
    tracker.serial = "LHR-TRACKER-01";
    tracker.displayName = "Waist Tracker";
    tracker.role = "Waist";
    tracker.modelName = "VIVE Tracker Pro";
    tracker.renderModelName = "vr_tracker_vive_3_0";
    tracker.manufacturerName = "HTC";
    tracker.recordEnabled = true;
    session.devices.push_back(tracker);

    const ovtr::SessionManifestStats stats{
        2,
        0.022222,
        0,
        true,
    };

    const std::string json = ovtr::makeManifestJson(session, stats);
    require(json.find("\"format\": \"OpenVRTrackerRecorderSession\"") != std::string::npos, "manifest format missing");
    require(json.find("\"finalized\": true") != std::string::npos, "manifest finalized flag missing");
    require(json.find("\"devices\": [") != std::string::npos, "manifest devices array missing");
    require(json.find("\"deviceClass\": \"GenericTracker\"") != std::string::npos, "manifest device class missing");
    require(json.find("\"serial\": \"LHR-TRACKER-01\"") != std::string::npos, "manifest device serial missing");
    require(json.find("\"displayName\": \"Waist Tracker\"") != std::string::npos, "manifest device name missing");
    require(json.find("\"video\": \"video.mp4\"") != std::string::npos, "manifest video path missing");
    require(json.find("\"modelName\": \"VIVE Tracker Pro\"") != std::string::npos, "manifest model name missing");
    require(
        json.find("\"renderModelName\": \"vr_tracker_vive_3_0\"") != std::string::npos,
        "manifest render model name missing"
    );

    const std::filesystem::path testDirectory =
        std::filesystem::current_path() / ".tmp_session_manifest_recovery";
    std::error_code ignored;
    std::filesystem::remove_all(testDirectory, ignored);
    std::string error;
    require(
        ovtr::writeManifestJson(
            session, stats, testDirectory / "manifest.json", error),
        "writing recovery manifest failed: " + error);
    require(
        ovtr::removeVideoReferenceFromSessionManifest(
            testDirectory / "manifest.json", error),
        "removing failed video reference failed: " + error);
    ovtr::RecordingSession recovered;
    ovtr::SessionManifestStats recoveredStats;
    require(
        ovtr::readManifestJson(
            testDirectory / "manifest.json", recovered, recoveredStats, error),
        "reading recovered manifest failed: " + error);
    require(recovered.videoPath.empty(), "recovered manifest should omit failed video");
    require(recoveredStats.finalized, "tracker-only recovery should preserve finalized stats");
    std::filesystem::remove_all(testDirectory, ignored);
}

} // namespace ovtr::test
