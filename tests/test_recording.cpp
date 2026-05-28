#include "TestCases.h"
#include "TestSupport.h"

#include "recording/RecordingController.h"

#include <filesystem>

namespace ovtr::test {

void testRecordingController()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_controller_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);

    ovtr::RecordingSession session;
    session.sessionId = "controller-test";
    session.sessionName = "Controller Test Session";
    session.createdAtUtc = "2026-05-25T00:00:00Z";
    session.appVersion = "0.1.0";

    ovtr::RecordingController controller;
    const ovtr::RecordingStartOptions options{testDir, session};
    require(controller.start(options), "recording controller start failed: " + controller.lastError());
    require(controller.state() == ovtr::RecorderState::Recording, "controller should be recording");
    require(controller.appendFrame(makeTestFrame(0)), "controller append failed: " + controller.lastError());
    require(controller.frameCount() == 1, "controller frame count mismatch");
    require(controller.stop(0.011111, 0), "controller stop failed: " + controller.lastError());
    require(controller.state() == ovtr::RecorderState::Idle, "controller should return to idle after stop");
    require(std::filesystem::exists(testDir / "manifest.json"), "controller did not write manifest");
    require(std::filesystem::exists(testDir / "frames.bin"), "controller did not write frames");
    require(std::filesystem::exists(testDir / "frame_index.bin"), "controller did not write frame index");

    std::filesystem::remove_all(testDir, ignored);
}

} // namespace ovtr::test
