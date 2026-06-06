#include "recording/SessionManifest.h"

#include "import/GltfJson.h"
#include "import/GltfJsonUtils.h"

#include <fstream>
#include <sstream>

namespace ovtr {
namespace {

DeviceClass deviceClassFromString(const std::string& value) noexcept
{
    if (value == "HMD") {
        return DeviceClass::Hmd;
    }
    if (value == "Controller") {
        return DeviceClass::Controller;
    }
    if (value == "GenericTracker") {
        return DeviceClass::GenericTracker;
    }
    if (value == "TrackingReference") {
        return DeviceClass::TrackingReference;
    }
    if (value == "Other") {
        return DeviceClass::Other;
    }
    return DeviceClass::Invalid;
}

bool jsonBoolMember(const JsonValue& object, const char* key, const bool defaultValue)
{
    const JsonValue* value = object.find(key);
    return value && value->type == JsonValue::Type::Bool ? value->boolValue : defaultValue;
}

std::uint32_t jsonUintMember(const JsonValue& object, const char* key)
{
    const int value = jsonIntMember(object, key, 0);
    return value > 0 ? static_cast<std::uint32_t>(value) : 0u;
}

std::uint64_t jsonUint64Member(const JsonValue& object, const char* key)
{
    return static_cast<std::uint64_t>(jsonSizeMember(object, key, 0));
}

DeviceDescriptor readDevice(const JsonValue& value)
{
    DeviceDescriptor device;
    device.id = jsonUintMember(value, "id");
    device.runtimeIndex = jsonUintMember(value, "runtimeIndex");
    device.deviceClass = deviceClassFromString(jsonStringMember(value, "deviceClass"));
    device.serial = jsonStringMember(value, "serial");
    device.displayName = jsonStringMember(value, "displayName");
    device.role = jsonStringMember(value, "role");
    device.modelName = jsonStringMember(value, "modelName");
    device.renderModelName = jsonStringMember(value, "renderModelName");
    device.manufacturerName = jsonStringMember(value, "manufacturerName");
    device.recordEnabled = jsonBoolMember(value, "recordEnabled", true);
    return device;
}

std::string readTextFile(const std::filesystem::path& path, std::string& outError)
{
    std::ifstream input(path);
    if (!input.is_open()) {
        outError = "failed to open manifest file";
        return {};
    }
    std::ostringstream stream;
    stream << input.rdbuf();
    if (!input.good() && !input.eof()) {
        outError = "failed to read manifest file";
        return {};
    }
    return stream.str();
}

} // namespace

bool readManifestJson(
    const std::filesystem::path& path,
    RecordingSession& outSession,
    SessionManifestStats& outStats,
    std::string& outError
)
{
    outError.clear();
    const std::string text = readTextFile(path, outError);
    if (!outError.empty()) {
        return false;
    }

    JsonValue root;
    JsonParser parser(text);
    if (!parser.parse(root) || root.type != JsonValue::Type::Object) {
        outError = "failed to parse manifest JSON: " + parser.error();
        return false;
    }

    outSession = RecordingSession{};
    outStats = SessionManifestStats{};
    outSession.sessionId = jsonStringMember(root, "sessionId");
    outSession.sessionName = jsonStringMember(root, "sessionName", outSession.sessionId);
    outSession.createdAtUtc = jsonStringMember(root, "createdAtUtc");
    outSession.appVersion = jsonStringMember(root, "appVersion");
    if (const JsonValue* sampleRate = root.find("sampleRate")) {
        jsonNumber(*sampleRate, outSession.targetSampleRate);
    }
    outSession.trackingUniverse = jsonStringMember(root, "trackingUniverse", "Standing");

    if (const JsonValue* files = root.find("files"); files && files->type == JsonValue::Type::Object) {
        outSession.framesPath = jsonStringMember(*files, "frames");
        outSession.frameIndexPath = jsonStringMember(*files, "frameIndex");
    }
    if (const JsonValue* stats = root.find("stats"); stats && stats->type == JsonValue::Type::Object) {
        outStats.frameCount = jsonUint64Member(*stats, "frameCount");
        const JsonValue* duration = stats->find("durationSeconds");
        if (duration) {
            jsonNumber(*duration, outStats.durationSeconds);
        }
        outStats.droppedFrames = jsonUint64Member(*stats, "droppedFrames");
    }
    outStats.finalized = jsonBoolMember(root, "finalized", false);

    if (const JsonValue* devices = root.find("devices"); devices && devices->type == JsonValue::Type::Array) {
        for (const JsonValue& device : devices->arrayValue) {
            if (device.type == JsonValue::Type::Object) {
                outSession.devices.push_back(readDevice(device));
            }
        }
    }
    return true;
}

} // namespace ovtr
