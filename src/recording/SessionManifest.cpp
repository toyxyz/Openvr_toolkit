#include "recording/SessionManifest.h"

#include "data/SessionTypes.h"
#include "util/BinaryBuffer.h"
#include "util/JsonWriter.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace ovtr {

namespace {

void writeJsonStringField(std::ostringstream& json, const std::string& name, const std::string& value, const bool comma)
{
    json << "  \"" << name << "\": \"" << escapeJsonString(value) << "\"";
    if (comma) {
        json << ',';
    }
    json << '\n';
}

void writeIndentedJsonStringField(
    std::ostringstream& json,
    const std::string& indent,
    const std::string& name,
    const std::string& value,
    const bool comma
)
{
    json << indent << "\"" << name << "\": \"" << escapeJsonString(value) << "\"";
    if (comma) {
        json << ',';
    }
    json << '\n';
}

void writeDeviceJson(std::ostringstream& json, const DeviceDescriptor& device, const bool comma)
{
    json << "    {\n";
    json << "      \"id\": " << device.id << ",\n";
    json << "      \"runtimeIndex\": " << device.runtimeIndex << ",\n";
    writeIndentedJsonStringField(json, "      ", "deviceClass", toString(device.deviceClass), true);
    writeIndentedJsonStringField(json, "      ", "serial", device.serial, true);
    writeIndentedJsonStringField(json, "      ", "displayName", device.displayName, true);
    writeIndentedJsonStringField(json, "      ", "role", device.role, true);
    writeIndentedJsonStringField(json, "      ", "modelName", device.modelName, true);
    writeIndentedJsonStringField(json, "      ", "renderModelName", device.renderModelName, true);
    writeIndentedJsonStringField(json, "      ", "manufacturerName", device.manufacturerName, true);
    json << "      \"recordEnabled\": " << (device.recordEnabled ? "true" : "false") << '\n';
    json << "    }";
    if (comma) {
        json << ',';
    }
    json << '\n';
}

void writeDevicesJson(std::ostringstream& json, const std::vector<DeviceDescriptor>& devices)
{
    json << "  \"devices\": [\n";
    for (std::size_t index = 0; index < devices.size(); ++index) {
        writeDeviceJson(json, devices[index], index + 1 < devices.size());
    }
    json << "  ],\n";
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
    writeDevicesJson(json, session.devices);
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

    std::string directoryError;
    if (!ensureParentDirectory(path, &directoryError)) {
        outError = "failed to create manifest directory: " + directoryError;
        return false;
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
