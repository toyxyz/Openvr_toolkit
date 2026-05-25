#include "recording/SessionManifest.h"

#include "data/SessionTypes.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace ovtr {

namespace {

std::string escapeJsonString(const std::string& value)
{
    std::ostringstream escaped;
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            escaped << "\\\\";
            break;
        case '"':
            escaped << "\\\"";
            break;
        case '\n':
            escaped << "\\n";
            break;
        case '\r':
            escaped << "\\r";
            break;
        case '\t':
            escaped << "\\t";
            break;
        default:
            escaped << ch;
            break;
        }
    }
    return escaped.str();
}

void writeJsonStringField(std::ostringstream& json, const std::string& name, const std::string& value, const bool comma)
{
    json << "  \"" << name << "\": \"" << escapeJsonString(value) << "\"";
    if (comma) {
        json << ',';
    }
    json << '\n';
}

} // namespace

std::string makeManifestJson(const RecordingSession& session, const SessionManifestStats& stats)
{
    std::ostringstream json;
    json << "{\n";
    writeJsonStringField(json, "format", "OpenVRTrackerRecorderSession", true);
    json << "  \"formatVersion\": 1,\n";
    writeJsonStringField(json, "sessionId", session.sessionId, true);
    writeJsonStringField(json, "sessionName", session.sessionName, true);
    writeJsonStringField(json, "createdAtUtc", session.createdAtUtc, true);
    writeJsonStringField(json, "appVersion", session.appVersion, true);
    json << "  \"sampleRate\": " << session.targetSampleRate << ",\n";
    writeJsonStringField(json, "trackingUniverse", session.trackingUniverse, true);
    json << "  \"coordinateSystem\": {\n";
    writeJsonStringField(json, "name", session.coordinateSystem, true);
    writeJsonStringField(json, "unit", session.unit, true);
    writeJsonStringField(json, "upAxis", "Y", true);
    writeJsonStringField(json, "forwardAxis", "-Z", false);
    json << "  },\n";
    json << "  \"files\": {\n";
    writeJsonStringField(json, "frames", session.framesPath.generic_string(), true);
    writeJsonStringField(json, "frameIndex", session.frameIndexPath.generic_string(), false);
    json << "  },\n";
    json << "  \"stats\": {\n";
    json << "    \"frameCount\": " << stats.frameCount << ",\n";
    json << "    \"durationSeconds\": " << stats.durationSeconds << ",\n";
    json << "    \"droppedFrames\": " << stats.droppedFrames << '\n';
    json << "  },\n";
    json << "  \"finalized\": " << (stats.finalized ? "true" : "false") << '\n';
    json << "}\n";
    return json.str();
}

bool writeManifestJson(
    const RecordingSession& session,
    const SessionManifestStats& stats,
    const std::filesystem::path& path,
    std::string& outError
)
{
    outError.clear();

    std::error_code error;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
            outError = "failed to create manifest directory: " + error.message();
            return false;
        }
    }

    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open()) {
        outError = "failed to open manifest file";
        return false;
    }

    output << makeManifestJson(session, stats);
    if (!output.good()) {
        outError = "failed to write manifest file";
        return false;
    }

    return true;
}

} // namespace ovtr

