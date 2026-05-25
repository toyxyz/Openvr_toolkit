#include "export/FbxAsciiExporter.h"

#include "export/RenderModelGeometry.h"
#include "math/QuaternionUtils.h"
#include "recording/BinarySessionReader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ovtr {

namespace {

constexpr std::int64_t kFbxTicksPerSecond = 46'186'158'000LL;
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = kPi * 2.0;
constexpr double kDegreesToRadians = 0.017453292519943295769;
constexpr double kRadiansToDegrees = 57.295779513082320876;
constexpr double kEulerHypotEpsilon = 0.0000375;

using Matrix3 = std::array<double, 9>;

struct PoseKey {
    double timeSeconds = 0.0;
    std::array<double, 3> translation{0.0, 0.0, 0.0};
    std::array<float, 4> rotationQuaternion{0.0f, 0.0f, 0.0f, 1.0f};
    std::array<double, 3> rotationDegrees{0.0, 0.0, 0.0};
    Matrix3 rotationMatrix{
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
    };
};

struct DeviceExport {
    DeviceDescriptor device;
    std::string nodeName;
    std::int64_t modelId = 0;
    std::int64_t geometryId = 0;
    std::int64_t translationNodeId = 0;
    std::int64_t rotationNodeId = 0;
    std::array<std::int64_t, 3> translationCurveIds{0, 0, 0};
    std::array<std::int64_t, 3> rotationCurveIds{0, 0, 0};
    std::vector<PoseKey> keys;
    RenderModelGeometry geometry;
};

struct FbxTimelineSettings {
    double frameRate = 0.0;
    int timeMode = 0;
    double customFrameRate = -1.0;
    std::int64_t startTime = 0;
    std::int64_t stopTime = 0;
};

std::string deviceClassPrefix(const DeviceClass deviceClass)
{
    switch (deviceClass) {
    case DeviceClass::Hmd:
        return "HMD";
    case DeviceClass::Controller:
        return "Controller";
    case DeviceClass::GenericTracker:
        return "Tracker";
    case DeviceClass::TrackingReference:
        return "TrackingReference";
    case DeviceClass::Invalid:
        return "Invalid";
    case DeviceClass::Other:
    default:
        return "Device";
    }
}

std::string sanitizeIdentifier(const std::string& input)
{
    std::string output;
    output.reserve(input.size());
    for (const char ch : input) {
        const unsigned char value = static_cast<unsigned char>(ch);
        if ((value >= 'A' && value <= 'Z') ||
            (value >= 'a' && value <= 'z') ||
            (value >= '0' && value <= '9')) {
            output.push_back(ch);
        } else {
            output.push_back('_');
        }
    }

    output.erase(
        std::unique(output.begin(), output.end(), [](const char a, const char b) {
            return a == '_' && b == '_';
        }),
        output.end()
    );

    while (!output.empty() && output.front() == '_') {
        output.erase(output.begin());
    }
    while (!output.empty() && output.back() == '_') {
        output.pop_back();
    }
    return output.empty() ? "Device" : output;
}

std::string escapeFbxString(const std::string& input)
{
    std::string output;
    output.reserve(input.size());
    for (const char ch : input) {
        output.push_back(ch == '"' ? '_' : ch);
    }
    return output;
}

std::int64_t toFbxTime(const double seconds)
{
    return static_cast<std::int64_t>(std::llround(seconds * static_cast<double>(kFbxTicksPerSecond)));
}

bool nearlyEqual(const double left, const double right)
{
    return std::fabs(left - right) < 0.0001;
}

double clamp01(const double value)
{
    return std::clamp(value, 0.0, 1.0);
}

std::array<double, 3> lerpVector(
    const std::array<double, 3>& from,
    const std::array<double, 3>& to,
    const double factor
)
{
    return {
        from[0] + (to[0] - from[0]) * factor,
        from[1] + (to[1] - from[1]) * factor,
        from[2] + (to[2] - from[2]) * factor,
    };
}

std::array<double, 3> eulerDegreesToRadians(const std::array<double, 3>& eulerDegrees)
{
    return {
        eulerDegrees[0] * kDegreesToRadians,
        eulerDegrees[1] * kDegreesToRadians,
        eulerDegrees[2] * kDegreesToRadians,
    };
}

std::array<double, 3> eulerRadiansToDegrees(const std::array<double, 3>& eulerRadians)
{
    return {
        eulerRadians[0] * kRadiansToDegrees,
        eulerRadians[1] * kRadiansToDegrees,
        eulerRadians[2] * kRadiansToDegrees,
    };
}

Matrix3 quaternionToMatrix3(const std::array<float, 4>& quaternion)
{
    const std::array<float, 4> q = normalizeQuaternion(quaternion);
    const double x = q[0];
    const double y = q[1];
    const double z = q[2];
    const double w = q[3];

    return {
        1.0 - 2.0 * (y * y + z * z), 2.0 * (x * y - z * w), 2.0 * (x * z + y * w),
        2.0 * (x * y + z * w), 1.0 - 2.0 * (x * x + z * z), 2.0 * (y * z - x * w),
        2.0 * (x * z - y * w), 2.0 * (y * z + x * w), 1.0 - 2.0 * (x * x + y * y),
    };
}

std::array<std::array<double, 3>, 2> matrix3ToEulerXyzCandidates(const Matrix3& matrix)
{
    const double cy = std::hypot(matrix[0], matrix[3]);
    std::array<double, 3> primary{};
    std::array<double, 3> secondary{};

    if (cy > kEulerHypotEpsilon) {
        primary = {
            std::atan2(matrix[7], matrix[8]),
            std::atan2(-matrix[6], cy),
            std::atan2(matrix[3], matrix[0]),
        };
        secondary = {
            std::atan2(-matrix[7], -matrix[8]),
            std::atan2(-matrix[6], -cy),
            std::atan2(-matrix[3], -matrix[0]),
        };
    } else {
        primary = {
            std::atan2(-matrix[5], matrix[4]),
            std::atan2(-matrix[6], cy),
            0.0,
        };
        secondary = primary;
    }

    return {primary, secondary};
}

double squaredEulerDistance(const std::array<double, 3>& left, const std::array<double, 3>& right)
{
    double distance = 0.0;
    for (int axis = 0; axis < 3; ++axis) {
        const double delta = left[axis] - right[axis];
        distance += delta * delta;
    }
    return distance;
}

std::array<double, 3> nearestEulerPeriod(const std::array<double, 3>& euler, const std::array<double, 3>& reference)
{
    std::array<double, 3> nearest = euler;
    for (int axis = 0; axis < 3; ++axis) {
        nearest[axis] += std::round((reference[axis] - nearest[axis]) / kTwoPi) * kTwoPi;
    }
    return nearest;
}

std::array<double, 3> compatibleEulerFromMatrix(const Matrix3& matrix, const std::array<double, 3>& previous)
{
    const auto baseCandidates = matrix3ToEulerXyzCandidates(matrix);
    std::array<double, 3> best = nearestEulerPeriod(baseCandidates[0], previous);
    double bestDistance = squaredEulerDistance(best, previous);

    for (const std::array<double, 3>& baseCandidate : baseCandidates) {
        const std::array<double, 3> centered = nearestEulerPeriod(baseCandidate, previous);
        for (int xOffset = -1; xOffset <= 1; ++xOffset) {
            for (int yOffset = -1; yOffset <= 1; ++yOffset) {
                for (int zOffset = -1; zOffset <= 1; ++zOffset) {
                    const std::array<double, 3> candidate{
                        centered[0] + static_cast<double>(xOffset) * kTwoPi,
                        centered[1] + static_cast<double>(yOffset) * kTwoPi,
                        centered[2] + static_cast<double>(zOffset) * kTwoPi,
                    };
                    const double distance = squaredEulerDistance(candidate, previous);
                    if (distance < bestDistance) {
                        best = candidate;
                        bestDistance = distance;
                    }
                }
            }
        }
    }

    return best;
}

void applyEulerDiscontinuityFilter(std::vector<PoseKey>& keys)
{
    if (keys.size() < 2) {
        return;
    }

    std::vector<std::array<double, 3>> eulers;
    eulers.reserve(keys.size());
    for (const PoseKey& key : keys) {
        eulers.push_back(eulerDegreesToRadians(key.rotationDegrees));
    }

    for (std::size_t index = 1; index < eulers.size(); ++index) {
        eulers[index] = compatibleEulerFromMatrix(keys[index].rotationMatrix, eulers[index - 1]);
    }

    for (std::size_t index = 1; index < eulers.size(); ++index) {
        for (int axis = 0; axis < 3; ++axis) {
            const double diff = eulers[index][axis] - eulers[index - 1][axis];
            if (std::fabs(diff) > kPi) {
                eulers[index][axis] -= std::round(diff / kTwoPi) * kTwoPi;
            }
        }
    }

    for (std::size_t index = 0; index < keys.size(); ++index) {
        keys[index].rotationDegrees = eulerRadiansToDegrees(eulers[index]);
    }
}

std::array<double, 3> convertVector(const std::array<double, 3>& value, const FbxCoordinatePolicy policy)
{
    if (policy == FbxCoordinatePolicy::Blender) {
        return {value[0], -value[2], value[1]};
    }
    return value;
}

std::array<float, 3> convertVector(const std::array<float, 3>& value, const FbxCoordinatePolicy policy)
{
    if (policy == FbxCoordinatePolicy::Blender) {
        return {value[0], -value[2], value[1]};
    }
    return value;
}

std::array<float, 4> multiplyQuaternions(const std::array<float, 4>& left, const std::array<float, 4>& right)
{
    return {
        left[3] * right[0] + left[0] * right[3] + left[1] * right[2] - left[2] * right[1],
        left[3] * right[1] - left[0] * right[2] + left[1] * right[3] + left[2] * right[0],
        left[3] * right[2] + left[0] * right[1] - left[1] * right[0] + left[2] * right[3],
        left[3] * right[3] - left[0] * right[0] - left[1] * right[1] - left[2] * right[2],
    };
}

std::array<float, 4> convertQuaternion(const std::array<float, 4>& quaternion, const FbxCoordinatePolicy policy)
{
    const std::array<float, 4> source = normalizeQuaternion(quaternion);
    if (policy != FbxCoordinatePolicy::Blender) {
        return source;
    }

    constexpr float halfSqrt = 0.7071067811865476f;
    constexpr std::array<float, 4> openVrToBlender{halfSqrt, 0.0f, 0.0f, halfSqrt};
    constexpr std::array<float, 4> blenderToOpenVr{-halfSqrt, 0.0f, 0.0f, halfSqrt};
    return normalizeQuaternion(multiplyQuaternions(multiplyQuaternions(openVrToBlender, source), blenderToOpenVr));
}

std::array<float, 4> slerpQuaternion(
    const std::array<float, 4>& from,
    const std::array<float, 4>& to,
    const double factor
)
{
    std::array<float, 4> start = normalizeQuaternion(from);
    std::array<float, 4> end = normalizeQuaternion(to);
    double dot =
        static_cast<double>(start[0]) * end[0] +
        static_cast<double>(start[1]) * end[1] +
        static_cast<double>(start[2]) * end[2] +
        static_cast<double>(start[3]) * end[3];

    if (dot < 0.0) {
        for (float& component : end) {
            component = -component;
        }
        dot = -dot;
    }

    if (dot > 0.9995) {
        std::array<float, 4> result{};
        for (int axis = 0; axis < 4; ++axis) {
            result[axis] = static_cast<float>(start[axis] + (end[axis] - start[axis]) * factor);
        }
        return normalizeQuaternion(result);
    }

    dot = std::clamp(dot, -1.0, 1.0);
    const double theta = std::acos(dot);
    const double sinTheta = std::sin(theta);
    const double startWeight = std::sin((1.0 - factor) * theta) / sinTheta;
    const double endWeight = std::sin(factor * theta) / sinTheta;

    return normalizeQuaternion({
        static_cast<float>(start[0] * startWeight + end[0] * endWeight),
        static_cast<float>(start[1] * startWeight + end[1] * endWeight),
        static_cast<float>(start[2] * startWeight + end[2] * endWeight),
        static_cast<float>(start[3] * startWeight + end[3] * endWeight),
    });
}

PoseKey makeInterpolatedPoseKey(const PoseKey& from, const PoseKey& to, const double timeSeconds)
{
    const double duration = to.timeSeconds - from.timeSeconds;
    const double factor = duration > 0.0 ? clamp01((timeSeconds - from.timeSeconds) / duration) : 0.0;

    PoseKey key;
    key.timeSeconds = timeSeconds;
    key.translation = lerpVector(from.translation, to.translation, factor);
    key.rotationQuaternion = slerpQuaternion(from.rotationQuaternion, to.rotationQuaternion, factor);
    key.rotationMatrix = quaternionToMatrix3(key.rotationQuaternion);
    key.rotationDegrees = eulerRadiansToDegrees(matrix3ToEulerXyzCandidates(key.rotationMatrix)[0]);
    return key;
}

void resamplePoseKeys(std::vector<PoseKey>& keys, const double sampleRate)
{
    if (sampleRate <= 0.0 || keys.size() < 2) {
        return;
    }

    const double interval = 1.0 / sampleRate;
    const double startTime = keys.front().timeSeconds;
    const double endTime = keys.back().timeSeconds;
    if (!std::isfinite(interval) || interval <= 0.0 || endTime <= startTime) {
        return;
    }

    std::vector<PoseKey> source = std::move(keys);
    std::vector<PoseKey> resampled;
    const std::size_t estimatedCount = static_cast<std::size_t>(std::floor((endTime - startTime) / interval)) + 2;
    resampled.reserve(estimatedCount);

    std::size_t upperIndex = 1;
    constexpr double epsilon = 1.0e-9;
    for (double time = startTime; time <= endTime + epsilon; time += interval) {
        const double sampleTime = std::min(time, endTime);
        while (upperIndex + 1 < source.size() && source[upperIndex].timeSeconds < sampleTime) {
            ++upperIndex;
        }
        resampled.push_back(makeInterpolatedPoseKey(source[upperIndex - 1], source[upperIndex], sampleTime));
    }

    if (!resampled.empty() && endTime - resampled.back().timeSeconds > interval * 0.25) {
        resampled.push_back(source.back());
    }

    keys = std::move(resampled);
}

int fbxTimeModeForFrameRate(const double frameRate)
{
    if (nearlyEqual(frameRate, 120.0)) {
        return 1;
    }
    if (nearlyEqual(frameRate, 100.0)) {
        return 2;
    }
    if (nearlyEqual(frameRate, 60.0)) {
        return 3;
    }
    if (nearlyEqual(frameRate, 50.0)) {
        return 4;
    }
    if (nearlyEqual(frameRate, 48.0)) {
        return 5;
    }
    if (nearlyEqual(frameRate, 30.0)) {
        return 6;
    }
    if (nearlyEqual(frameRate, 25.0)) {
        return 10;
    }
    if (nearlyEqual(frameRate, 24.0)) {
        return 11;
    }
    if (nearlyEqual(frameRate, 1000.0)) {
        return 12;
    }
    if (nearlyEqual(frameRate, 96.0)) {
        return 15;
    }
    if (nearlyEqual(frameRate, 72.0)) {
        return 16;
    }
    if (nearlyEqual(frameRate, 60000.0 / 1001.0)) {
        return 17;
    }
    return 14;
}

double inferFrameRateFromKeys(const std::vector<DeviceExport>& devices)
{
    for (const DeviceExport& device : devices) {
        if (device.keys.size() < 2) {
            continue;
        }

        const double interval = device.keys[1].timeSeconds - device.keys[0].timeSeconds;
        if (interval > 0.0 && std::isfinite(interval)) {
            return 1.0 / interval;
        }
    }

    return 0.0;
}

FbxTimelineSettings makeTimelineSettings(
    const std::vector<DeviceExport>& devices,
    const double requestedFrameRate,
    const double sessionFrameRate
)
{
    FbxTimelineSettings settings;
    if (requestedFrameRate > 0.0) {
        settings.frameRate = requestedFrameRate;
    } else if (sessionFrameRate > 0.0) {
        settings.frameRate = sessionFrameRate;
    } else {
        settings.frameRate = inferFrameRateFromKeys(devices);
    }

    if (settings.frameRate > 0.0 && std::isfinite(settings.frameRate)) {
        settings.timeMode = fbxTimeModeForFrameRate(settings.frameRate);
        settings.customFrameRate = settings.timeMode == 14 ? settings.frameRate : -1.0;
    }

    double startSeconds = 0.0;
    double stopSeconds = 0.0;
    bool hasKey = false;
    for (const DeviceExport& device : devices) {
        if (device.keys.empty()) {
            continue;
        }
        if (!hasKey) {
            startSeconds = device.keys.front().timeSeconds;
            stopSeconds = device.keys.back().timeSeconds;
            hasKey = true;
        } else {
            startSeconds = std::min(startSeconds, device.keys.front().timeSeconds);
            stopSeconds = std::max(stopSeconds, device.keys.back().timeSeconds);
        }
    }

    settings.startTime = toFbxTime(hasKey ? startSeconds : 0.0);
    settings.stopTime = toFbxTime(hasKey ? stopSeconds : 0.0);
    return settings;
}

std::string joinedDoubles(const std::vector<double>& values)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(9);
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            stream << ",";
        }
        stream << values[i];
    }
    return stream.str();
}

std::string joinedInt64(const std::vector<std::int64_t>& values)
{
    std::ostringstream stream;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            stream << ",";
        }
        stream << values[i];
    }
    return stream.str();
}

std::string joinedInt32(const std::vector<std::int32_t>& values)
{
    std::ostringstream stream;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            stream << ",";
        }
        stream << values[i];
    }
    return stream.str();
}

std::vector<double> collectGeometryVertices(const RenderModelGeometry& geometry)
{
    std::vector<double> values;
    values.reserve(geometry.vertices.size() * 3);
    for (const RenderModelVertex& vertex : geometry.vertices) {
        values.push_back(vertex.position[0]);
        values.push_back(vertex.position[1]);
        values.push_back(vertex.position[2]);
    }
    return values;
}

std::vector<double> collectGeometryNormals(const RenderModelGeometry& geometry)
{
    std::vector<double> values;
    values.reserve(geometry.indices.size() * 3);
    for (const std::uint16_t index : geometry.indices) {
        if (index >= geometry.vertices.size()) {
            continue;
        }
        const RenderModelVertex& vertex = geometry.vertices[index];
        values.push_back(vertex.normal[0]);
        values.push_back(vertex.normal[1]);
        values.push_back(vertex.normal[2]);
    }
    return values;
}

std::vector<std::int32_t> collectPolygonVertexIndex(const RenderModelGeometry& geometry)
{
    std::vector<std::int32_t> values;
    values.reserve(geometry.indices.size());
    for (std::size_t i = 0; i + 2 < geometry.indices.size(); i += 3) {
        values.push_back(static_cast<std::int32_t>(geometry.indices[i]));
        values.push_back(static_cast<std::int32_t>(geometry.indices[i + 1]));
        values.push_back(-static_cast<std::int32_t>(geometry.indices[i + 2]) - 1);
    }
    return values;
}

void convertGeometry(RenderModelGeometry& geometry, const FbxCoordinatePolicy policy)
{
    if (policy != FbxCoordinatePolicy::Blender) {
        return;
    }

    for (RenderModelVertex& vertex : geometry.vertices) {
        vertex.position = convertVector(vertex.position, policy);
        vertex.normal = convertVector(vertex.normal, policy);
    }
}

RenderModelGeometry loadFbxGeometry(const std::string& renderModelName, const FbxCoordinatePolicy policy)
{
    RenderModelGeometry geometry = loadSteamVRRenderModelGeometry(renderModelName);
    convertGeometry(geometry, policy);
    return geometry;
}

void writeHeader(std::ostream& out, const FbxCoordinatePolicy policy, const FbxTimelineSettings& timeline)
{
    out << "; FBX 7.4.0 project file\n";
    out << "; Created by OpenVR Tracker Recorder\n";
    out << "FBXHeaderExtension:  {\n";
    out << "    FBXHeaderVersion: 1003\n";
    out << "    FBXVersion: 7400\n";
    out << "    Creator: \"OpenVR Tracker Recorder\"\n";
    out << "}\n";
    out << "GlobalSettings:  {\n";
    out << "    Version: 1000\n";
    out << "    Properties70:  {\n";
    if (policy == FbxCoordinatePolicy::Blender) {
        out << "        P: \"UpAxis\", \"int\", \"Integer\", \"\",2\n";
        out << "        P: \"UpAxisSign\", \"int\", \"Integer\", \"\",1\n";
        out << "        P: \"FrontAxis\", \"int\", \"Integer\", \"\",1\n";
        out << "        P: \"FrontAxisSign\", \"int\", \"Integer\", \"\",-1\n";
        out << "        P: \"CoordAxis\", \"int\", \"Integer\", \"\",0\n";
        out << "        P: \"CoordAxisSign\", \"int\", \"Integer\", \"\",1\n";
    } else {
        out << "        P: \"UpAxis\", \"int\", \"Integer\", \"\",1\n";
        out << "        P: \"UpAxisSign\", \"int\", \"Integer\", \"\",1\n";
        out << "        P: \"FrontAxis\", \"int\", \"Integer\", \"\",2\n";
        out << "        P: \"FrontAxisSign\", \"int\", \"Integer\", \"\",1\n";
        out << "        P: \"CoordAxis\", \"int\", \"Integer\", \"\",0\n";
        out << "        P: \"CoordAxisSign\", \"int\", \"Integer\", \"\",1\n";
    }
    out << "        P: \"UnitScaleFactor\", \"double\", \"Number\", \"\",1\n";
    out << "        P: \"OriginalUnitScaleFactor\", \"double\", \"Number\", \"\",1\n";
    out << "        P: \"AmbientColor\", \"ColorRGB\", \"Color\", \"\",0,0,0\n";
    out << "        P: \"DefaultCamera\", \"KString\", \"\", \"\", \"Producer Perspective\"\n";
    out << "        P: \"TimeMode\", \"enum\", \"\", \"\"," << timeline.timeMode << "\n";
    out << "        P: \"TimeProtocol\", \"enum\", \"\", \"\",2\n";
    out << "        P: \"SnapOnFrameMode\", \"enum\", \"\", \"\",0\n";
    out << "        P: \"TimeSpanStart\", \"KTime\", \"Time\", \"\"," << timeline.startTime << "\n";
    out << "        P: \"TimeSpanStop\", \"KTime\", \"Time\", \"\"," << timeline.stopTime << "\n";
    out << "        P: \"CustomFrameRate\", \"double\", \"Number\", \"\"," << timeline.customFrameRate << "\n";
    out << "        P: \"TimeMarker\", \"Compound\", \"\", \"\"\n";
    out << "        P: \"CurrentTimeMarker\", \"int\", \"Integer\", \"\",-1\n";
    out << "    }\n";
    out << "}\n";
}

void writeModel(std::ostream& out, const DeviceExport& device)
{
    const PoseKey defaultKey = device.keys.empty() ? PoseKey{} : device.keys.front();
    out << "    Model: " << device.modelId << ", \"Model::" << escapeFbxString(device.nodeName) << "\", \"Mesh\" {\n";
    out << "        Version: 232\n";
    out << "        Properties70:  {\n";
    out << "            P: \"Lcl Translation\", \"Lcl Translation\", \"\", \"A\","
        << defaultKey.translation[0] << "," << defaultKey.translation[1] << "," << defaultKey.translation[2] << "\n";
    out << "            P: \"Lcl Rotation\", \"Lcl Rotation\", \"\", \"A\","
        << defaultKey.rotationDegrees[0] << "," << defaultKey.rotationDegrees[1] << "," << defaultKey.rotationDegrees[2] << "\n";
    out << "            P: \"RotationOrder\", \"enum\", \"\", \"\",0\n";
    out << "        }\n";
    out << "        Shading: T\n";
    out << "        Culling: \"CullingOff\"\n";
    out << "    }\n";
}

void writeRootModel(std::ostream& out, const std::int64_t rootModelId)
{
    out << "    Model: " << rootModelId << ", \"Model::OpenVR_Root\", \"Null\" {\n";
    out << "        Version: 232\n";
    out << "        Properties70:  {\n";
    out << "            P: \"Lcl Translation\", \"Lcl Translation\", \"\", \"A\",0,0,0\n";
    out << "            P: \"Lcl Rotation\", \"Lcl Rotation\", \"\", \"A\",0,0,0\n";
    out << "            P: \"Lcl Scaling\", \"Lcl Scaling\", \"\", \"A\",1,1,1\n";
    out << "        }\n";
    out << "        Shading: T\n";
    out << "        Culling: \"CullingOff\"\n";
    out << "    }\n";
}

void writeAnimationStack(std::ostream& out, const std::int64_t stackId, const FbxTimelineSettings& timeline)
{
    out << "    AnimationStack: " << stackId << ", \"AnimStack::Recorded Motion\", \"\" {\n";
    out << "        Properties70:  {\n";
    out << "            P: \"LocalStart\", \"KTime\", \"Time\", \"\"," << timeline.startTime << "\n";
    out << "            P: \"LocalStop\", \"KTime\", \"Time\", \"\"," << timeline.stopTime << "\n";
    out << "            P: \"ReferenceStart\", \"KTime\", \"Time\", \"\"," << timeline.startTime << "\n";
    out << "            P: \"ReferenceStop\", \"KTime\", \"Time\", \"\"," << timeline.stopTime << "\n";
    out << "        }\n";
    out << "    }\n";
}

void writeGeometry(std::ostream& out, const DeviceExport& device)
{
    if (!device.geometry.available) {
        return;
    }

    const std::vector<double> vertices = collectGeometryVertices(device.geometry);
    const std::vector<std::int32_t> polygonVertexIndex = collectPolygonVertexIndex(device.geometry);
    const std::vector<double> normals = collectGeometryNormals(device.geometry);

    out << "    Geometry: " << device.geometryId << ", \"Geometry::" << escapeFbxString(device.nodeName)
        << "_Geometry\", \"Mesh\" {\n";
    out << "        Vertices: *" << vertices.size() << " {\n";
    out << "            a: " << joinedDoubles(vertices) << "\n";
    out << "        }\n";
    out << "        PolygonVertexIndex: *" << polygonVertexIndex.size() << " {\n";
    out << "            a: " << joinedInt32(polygonVertexIndex) << "\n";
    out << "        }\n";
    out << "        LayerElementNormal: 0 {\n";
    out << "            Version: 101\n";
    out << "            Name: \"\"\n";
    out << "            MappingInformationType: \"ByPolygonVertex\"\n";
    out << "            ReferenceInformationType: \"Direct\"\n";
    out << "            Normals: *" << normals.size() << " {\n";
    out << "                a: " << joinedDoubles(normals) << "\n";
    out << "            }\n";
    out << "        }\n";
    out << "        Layer: 0 {\n";
    out << "            Version: 100\n";
    out << "            LayerElement:  {\n";
    out << "                Type: \"LayerElementNormal\"\n";
    out << "                TypedIndex: 0\n";
    out << "            }\n";
    out << "        }\n";
    out << "    }\n";
}

void writeCurve(std::ostream& out, const std::int64_t id, const std::string& name, const std::vector<PoseKey>& keys, const bool rotation, const int axis)
{
    std::vector<std::int64_t> times;
    std::vector<double> values;
    times.reserve(keys.size());
    values.reserve(keys.size());
    for (const PoseKey& key : keys) {
        times.push_back(toFbxTime(key.timeSeconds));
        values.push_back(rotation ? key.rotationDegrees[axis] : key.translation[axis]);
    }

    out << "    AnimationCurve: " << id << ", \"AnimCurve::" << name << "\", \"\" {\n";
    out << "        Default: 0\n";
    out << "        KeyVer: 4008\n";
    out << "        KeyTime: *" << times.size() << " {\n";
    out << "            a: " << joinedInt64(times) << "\n";
    out << "        }\n";
    out << "        KeyValueFloat: *" << values.size() << " {\n";
    out << "            a: " << joinedDoubles(values) << "\n";
    out << "        }\n";
    out << "        KeyAttrFlags: *1 {\n";
    out << "            a: 24836\n";
    out << "        }\n";
    out << "        KeyAttrDataFloat: *4 {\n";
    out << "            a: 0,0,9.999999747e-006,0\n";
    out << "        }\n";
    out << "        KeyAttrRefCount: *1 {\n";
    out << "            a: " << values.size() << "\n";
    out << "        }\n";
    out << "    }\n";
}

void writeCurveNode(std::ostream& out, const std::int64_t id, const std::string& name, const bool rotation)
{
    out << "    AnimationCurveNode: " << id << ", \"AnimCurveNode::" << name << "\", \"\" {\n";
    out << "        Properties70:  {\n";
    out << "            P: \"d|X\", \"Number\", \"\", \"A\",0\n";
    out << "            P: \"d|Y\", \"Number\", \"\", \"A\",0\n";
    out << "            P: \"d|Z\", \"Number\", \"\", \"A\",0\n";
    out << "        }\n";
    out << "        Channel: \"" << (rotation ? "R" : "T") << "\"\n";
    out << "    }\n";
}

void writeConnections(
    std::ostream& out,
    const std::vector<DeviceExport>& devices,
    const std::int64_t rootModelId,
    const std::int64_t stackId,
    const std::int64_t layerId
)
{
    out << "Connections:  {\n";
    out << "    C: \"OO\"," << rootModelId << ",0\n";
    out << "    C: \"OO\"," << layerId << "," << stackId << "\n";
    for (const DeviceExport& device : devices) {
        out << "    C: \"OO\"," << device.modelId << "," << rootModelId << "\n";
        if (device.geometry.available) {
            out << "    C: \"OO\"," << device.geometryId << "," << device.modelId << "\n";
        }
        out << "    C: \"OO\"," << device.translationNodeId << "," << layerId << "\n";
        out << "    C: \"OP\"," << device.translationNodeId << "," << device.modelId << ",\"Lcl Translation\"\n";
        out << "    C: \"OO\"," << device.rotationNodeId << "," << layerId << "\n";
        out << "    C: \"OP\"," << device.rotationNodeId << "," << device.modelId << ",\"Lcl Rotation\"\n";
        for (int axis = 0; axis < 3; ++axis) {
            out << "    C: \"OP\"," << device.translationCurveIds[axis] << "," << device.translationNodeId
                << ",\"d|" << static_cast<char>('X' + axis) << "\"\n";
            out << "    C: \"OP\"," << device.rotationCurveIds[axis] << "," << device.rotationNodeId
                << ",\"d|" << static_cast<char>('X' + axis) << "\"\n";
        }
    }
    out << "}\n";
}

} // namespace

std::string makeFbxSafeName(const DeviceDescriptor& device)
{
    std::string source = device.serial;
    if (source.empty()) {
        std::ostringstream stream;
        stream << "Device_" << device.runtimeIndex;
        source = stream.str();
    }

    return sanitizeIdentifier(deviceClassPrefix(device.deviceClass) + "_" + source);
}

std::array<double, 3> quaternionToEulerXyzDegrees(const std::array<float, 4>& quaternion)
{
    const Matrix3 matrix = quaternionToMatrix3(quaternion);
    return eulerRadiansToDegrees(matrix3ToEulerXyzCandidates(matrix)[0]);
}

ExportResult exportSessionToFbxAscii(const RecordingSession& session, const FbxExportOptions& options)
{
    ExportResult result;
    result.outputPath = options.outputPath;

    if (options.rotationOrder != FbxRotationOrder::XYZ) {
        result.error = "unsupported FBX export option";
        return result;
    }

    if (options.outputPath.empty()) {
        result.error = "FBX output path is empty";
        return result;
    }

    BinarySessionReader reader;
    if (!reader.open(session.framesPath, session.frameIndexPath)) {
        result.error = "failed to open recorded session: " + reader.lastError();
        return result;
    }

    std::vector<DeviceExport> devices;
    devices.reserve(session.devices.size());
    std::int64_t nextId = 1'000'000;
    for (const DeviceDescriptor& descriptor : session.devices) {
        if (!options.includeTrackingReference && descriptor.deviceClass == DeviceClass::TrackingReference) {
            continue;
        }

        DeviceExport device;
        device.device = descriptor;
        device.nodeName = makeFbxSafeName(descriptor);
        device.modelId = nextId++;
        device.geometryId = nextId++;
        device.translationNodeId = nextId++;
        device.rotationNodeId = nextId++;
        for (int axis = 0; axis < 3; ++axis) {
            device.translationCurveIds[axis] = nextId++;
            device.rotationCurveIds[axis] = nextId++;
        }
        if (options.includeGeometry) {
            device.geometry = loadFbxGeometry(descriptor.renderModelName, options.coordinatePolicy);
        }
        devices.push_back(std::move(device));
    }

    std::unordered_map<std::uint32_t, std::size_t> runtimeIndexToDevice;
    for (std::size_t i = 0; i < devices.size(); ++i) {
        runtimeIndexToDevice[devices[i].device.runtimeIndex] = i;
    }

    for (std::uint64_t frameIndex = 0; frameIndex < reader.frameCount(); ++frameIndex) {
        FrameSample frame;
        if (!reader.readFrame(frameIndex, frame)) {
            result.error = "failed to read recorded frame: " + reader.lastError();
            return result;
        }

        for (const PoseSample& pose : frame.poses) {
            if ((pose.flags & PoseFlagPoseValid) == 0) {
                continue;
            }

            const auto found = runtimeIndexToDevice.find(pose.runtimeIndex);
            if (found == runtimeIndexToDevice.end()) {
                continue;
            }

            PoseKey key;
            key.timeSeconds = frame.timeSeconds;
            const std::array<float, 3> translation = convertVector(pose.position, options.coordinatePolicy);
            key.translation = {
                static_cast<double>(translation[0]),
                static_cast<double>(translation[1]),
                static_cast<double>(translation[2]),
            };
            const std::array<float, 4> rotation = convertQuaternion(pose.rotation, options.coordinatePolicy);
            key.rotationQuaternion = rotation;
            key.rotationMatrix = quaternionToMatrix3(rotation);
            key.rotationDegrees = quaternionToEulerXyzDegrees(rotation);
            devices[found->second].keys.push_back(key);
        }
    }

    for (DeviceExport& device : devices) {
        resamplePoseKeys(device.keys, options.exportSampleRate);
        applyEulerDiscontinuityFilter(device.keys);
    }
    const FbxTimelineSettings timeline = makeTimelineSettings(
        devices,
        options.exportSampleRate,
        session.targetSampleRate
    );

    std::error_code error;
    std::filesystem::create_directories(options.outputPath.parent_path(), error);
    if (error) {
        result.error = "failed to create FBX export directory: " + error.message();
        return result;
    }

    std::ofstream out(options.outputPath, std::ios::trunc);
    if (!out.is_open()) {
        result.error = "failed to open FBX output file";
        return result;
    }

    const std::int64_t rootModelId = nextId++;
    const std::int64_t stackId = nextId++;
    const std::int64_t layerId = nextId++;

    writeHeader(out, options.coordinatePolicy, timeline);
    out << "Objects:  {\n";
    writeRootModel(out, rootModelId);
    for (const DeviceExport& device : devices) {
        writeGeometry(out, device);
        writeModel(out, device);
    }
    writeAnimationStack(out, stackId, timeline);
    out << "    AnimationLayer: " << layerId << ", \"AnimLayer::BaseLayer\", \"\" {\n";
    out << "    }\n";
    for (const DeviceExport& device : devices) {
        writeCurveNode(out, device.translationNodeId, device.nodeName + "_T", false);
        writeCurveNode(out, device.rotationNodeId, device.nodeName + "_R", true);
        for (int axis = 0; axis < 3; ++axis) {
            const std::string axisName(1, static_cast<char>('X' + axis));
            writeCurve(out, device.translationCurveIds[axis], device.nodeName + "_T_" + axisName, device.keys, false, axis);
            writeCurve(out, device.rotationCurveIds[axis], device.nodeName + "_R_" + axisName, device.keys, true, axis);
        }
    }
    out << "}\n";
    writeConnections(out, devices, rootModelId, stackId, layerId);

    if (!out.good()) {
        result.error = "failed while writing FBX output file";
        return result;
    }

    result.success = true;
    return result;
}

} // namespace ovtr
