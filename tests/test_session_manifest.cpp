#include "TestCases.h"
#include "TestSupport.h"

#include "recording/SessionManifest.h"

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

    const ovtr::SessionManifestStats stats{
        2,
        0.022222,
        0,
        true,
    };

    const std::string json = ovtr::makeManifestJson(session, stats);
    require(json.find("\"format\": \"OpenVRTrackerRecorderSession\"") != std::string::npos, "manifest format missing");
    require(json.find("\"finalized\": true") != std::string::npos, "manifest finalized flag missing");
}

} // namespace ovtr::test
