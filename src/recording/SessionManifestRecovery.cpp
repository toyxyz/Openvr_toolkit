#include "recording/SessionManifestRecovery.h"

#include "recording/SessionManifest.h"

namespace ovtr {

bool removeVideoReferenceFromSessionManifest(
    const std::filesystem::path& manifestPath,
    std::string& outError
)
{
    RecordingSession session;
    SessionManifestStats stats;
    if (!readManifestJson(manifestPath, session, stats, outError)) {
        outError = "Reading session manifest for video recovery failed: " + outError;
        return false;
    }
    session.videoPath.clear();
    if (!writeManifestJson(session, stats, manifestPath, outError)) {
        outError = "Writing tracker-only session manifest failed: " + outError;
        return false;
    }
    outError.clear();
    return true;
}

} // namespace ovtr
