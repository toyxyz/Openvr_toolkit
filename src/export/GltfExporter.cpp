#include "export/GltfExporter.h"

#include "math/QuaternionUtils.h"
#include "recording/BinarySessionReader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ovtr {

namespace {

constexpr int kGltfComponentFloat = 5126;
constexpr int kGltfComponentUnsignedShort = 5123;
constexpr int kGltfArrayBuffer = 34962;
constexpr int kGltfElementArrayBuffer = 34963;

struct GltfKey {
    double timeSeconds = 0.0;
    std::array<float, 3> translation{0.0f, 0.0f, 0.0f};
    std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};
};

struct GltfDevice {
    DeviceDescriptor device;
    std::string nodeName;
    int nodeIndex = -1;
    int meshIndex = -1;
    std::vector<GltfKey> keys;
    RenderModelGeometry geometry;
};

struct BufferView {
    std::size_t byteOffset = 0;
    std::size_t byteLength = 0;
    int target = 0;
};

struct Accessor {
    int bufferView = -1;
    int componentType = 5126;
    int count = 0;
    std::string type;
    std::vector<double> minValues;
    std::vector<double> maxValues;
};

struct MeshPrimitive {
    std::string name;
    int positionAccessor = -1;
    int normalAccessor = -1;
    int indexAccessor = -1;
};

struct AnimationTarget {
    int node = -1;
    int timeAccessor = -1;
    int translationAccessor = -1;
    int rotationAccessor = -1;
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

std::string makeGltfSafeName(const DeviceDescriptor& device)
{
    std::string source = device.serial;
    if (source.empty()) {
        std::ostringstream stream;
        stream << "Device_" << device.runtimeIndex;
        source = stream.str();
    }

    return sanitizeIdentifier(deviceClassPrefix(device.deviceClass) + "_" + source);
}

std::string escapeJsonString(const std::string& input)
{
    std::ostringstream stream;
    for (const unsigned char ch : input) {
        switch (ch) {
        case '\\':
            stream << "\\\\";
            break;
        case '"':
            stream << "\\\"";
            break;
        case '\b':
            stream << "\\b";
            break;
        case '\f':
            stream << "\\f";
            break;
        case '\n':
            stream << "\\n";
            break;
        case '\r':
            stream << "\\r";
            break;
        case '\t':
            stream << "\\t";
            break;
        default:
            if (ch < 0x20) {
                stream << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch)
                       << std::dec << std::setfill(' ');
            } else {
                stream << static_cast<char>(ch);
            }
            break;
        }
    }
    return stream.str();
}

double clamp01(const double value)
{
    return std::clamp(value, 0.0, 1.0);
}

std::array<float, 3> lerpVector(
    const std::array<float, 3>& from,
    const std::array<float, 3>& to,
    const double factor
)
{
    return {
        static_cast<float>(from[0] + (to[0] - from[0]) * factor),
        static_cast<float>(from[1] + (to[1] - from[1]) * factor),
        static_cast<float>(from[2] + (to[2] - from[2]) * factor),
    };
}

double quaternionDot(const std::array<float, 4>& left, const std::array<float, 4>& right)
{
    return static_cast<double>(left[0]) * right[0] +
           static_cast<double>(left[1]) * right[1] +
           static_cast<double>(left[2]) * right[2] +
           static_cast<double>(left[3]) * right[3];
}

std::array<float, 4> slerpQuaternion(
    const std::array<float, 4>& from,
    const std::array<float, 4>& to,
    const double factor
)
{
    std::array<float, 4> start = normalizeQuaternion(from);
    std::array<float, 4> end = normalizeQuaternion(to);
    double dot = quaternionDot(start, end);

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

GltfKey interpolateKey(const GltfKey& from, const GltfKey& to, const double timeSeconds)
{
    const double duration = to.timeSeconds - from.timeSeconds;
    const double factor = duration > 0.0 ? clamp01((timeSeconds - from.timeSeconds) / duration) : 0.0;

    GltfKey key;
    key.timeSeconds = timeSeconds;
    key.translation = lerpVector(from.translation, to.translation, factor);
    key.rotation = slerpQuaternion(from.rotation, to.rotation, factor);
    return key;
}

void makeQuaternionKeysContinuous(std::vector<GltfKey>& keys)
{
    for (std::size_t index = 1; index < keys.size(); ++index) {
        if (quaternionDot(keys[index - 1].rotation, keys[index].rotation) < 0.0) {
            for (float& component : keys[index].rotation) {
                component = -component;
            }
        }
    }
}

void resampleKeys(std::vector<GltfKey>& keys, const double sampleRate)
{
    if (sampleRate <= 0.0 || keys.size() < 2) {
        makeQuaternionKeysContinuous(keys);
        return;
    }

    const double interval = 1.0 / sampleRate;
    const double startTime = keys.front().timeSeconds;
    const double endTime = keys.back().timeSeconds;
    if (!std::isfinite(interval) || interval <= 0.0 || endTime <= startTime) {
        makeQuaternionKeysContinuous(keys);
        return;
    }

    std::vector<GltfKey> source = std::move(keys);
    std::vector<GltfKey> resampled;
    const std::size_t estimatedCount = static_cast<std::size_t>(std::floor((endTime - startTime) / interval)) + 2;
    resampled.reserve(estimatedCount);

    std::size_t upperIndex = 1;
    constexpr double epsilon = 1.0e-9;
    for (double time = startTime; time <= endTime + epsilon; time += interval) {
        const double sampleTime = std::min(time, endTime);
        while (upperIndex + 1 < source.size() && source[upperIndex].timeSeconds < sampleTime) {
            ++upperIndex;
        }
        resampled.push_back(interpolateKey(source[upperIndex - 1], source[upperIndex], sampleTime));
    }

    if (!resampled.empty() && endTime - resampled.back().timeSeconds > interval * 0.25) {
        resampled.push_back(source.back());
    }

    keys = std::move(resampled);
    makeQuaternionKeysContinuous(keys);
}

double globalStartTime(const std::vector<GltfDevice>& devices)
{
    double start = 0.0;
    bool found = false;
    for (const GltfDevice& device : devices) {
        if (device.keys.empty()) {
            continue;
        }
        if (!found) {
            start = device.keys.front().timeSeconds;
            found = true;
        } else {
            start = std::min(start, device.keys.front().timeSeconds);
        }
    }
    return found ? start : 0.0;
}

void alignBuffer(std::vector<std::uint8_t>& buffer)
{
    while ((buffer.size() % 4) != 0) {
        buffer.push_back(0);
    }
}

void appendUint32(std::vector<std::uint8_t>& bytes, const std::uint32_t value)
{
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16u) & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24u) & 0xffu));
}

void appendUint16(std::vector<std::uint8_t>& bytes, const std::uint16_t value)
{
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
}

void appendFloat(std::vector<std::uint8_t>& bytes, const float value)
{
    static_assert(sizeof(float) == sizeof(std::uint32_t));
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    appendUint32(bytes, bits);
}

int appendFloatBufferView(
    std::vector<std::uint8_t>& buffer,
    std::vector<BufferView>& bufferViews,
    const std::vector<float>& values,
    const int target = 0
)
{
    alignBuffer(buffer);
    const BufferView view{
        buffer.size(),
        values.size() * sizeof(float),
        target,
    };
    for (const float value : values) {
        appendFloat(buffer, value);
    }
    bufferViews.push_back(view);
    return static_cast<int>(bufferViews.size() - 1);
}

int appendUint16BufferView(
    std::vector<std::uint8_t>& buffer,
    std::vector<BufferView>& bufferViews,
    const std::vector<std::uint16_t>& values,
    const int target = 0
)
{
    alignBuffer(buffer);
    const BufferView view{
        buffer.size(),
        values.size() * sizeof(std::uint16_t),
        target,
    };
    for (const std::uint16_t value : values) {
        appendUint16(buffer, value);
    }
    bufferViews.push_back(view);
    return static_cast<int>(bufferViews.size() - 1);
}

int addAccessor(
    std::vector<Accessor>& accessors,
    const int bufferView,
    const int componentType,
    const int count,
    std::string type,
    std::vector<double> minValues = {},
    std::vector<double> maxValues = {}
)
{
    accessors.push_back({
        bufferView,
        componentType,
        count,
        std::move(type),
        std::move(minValues),
        std::move(maxValues),
    });
    return static_cast<int>(accessors.size() - 1);
}

std::string formatDouble(const double value)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(9) << value;
    return stream.str();
}

void writeJsonNumberArray(std::ostream& out, const std::vector<double>& values)
{
    out << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            out << ",";
        }
        out << formatDouble(values[i]);
    }
    out << "]";
}

void writeJsonFloatArray(std::ostream& out, const std::array<float, 3>& values)
{
    out << "[" << formatDouble(values[0]) << "," << formatDouble(values[1]) << "," << formatDouble(values[2]) << "]";
}

void writeJsonFloatArray(std::ostream& out, const std::array<float, 4>& values)
{
    out << "[" << formatDouble(values[0]) << "," << formatDouble(values[1]) << "," << formatDouble(values[2]) << ","
        << formatDouble(values[3]) << "]";
}

std::vector<double> minPositionBounds(const RenderModelGeometry& geometry)
{
    std::array<double, 3> bounds{
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max(),
    };
    for (const RenderModelVertex& vertex : geometry.vertices) {
        for (int axis = 0; axis < 3; ++axis) {
            bounds[axis] = std::min(bounds[axis], static_cast<double>(vertex.position[axis]));
        }
    }
    return {bounds[0], bounds[1], bounds[2]};
}

std::vector<double> maxPositionBounds(const RenderModelGeometry& geometry)
{
    std::array<double, 3> bounds{
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest(),
    };
    for (const RenderModelVertex& vertex : geometry.vertices) {
        for (int axis = 0; axis < 3; ++axis) {
            bounds[axis] = std::max(bounds[axis], static_cast<double>(vertex.position[axis]));
        }
    }
    return {bounds[0], bounds[1], bounds[2]};
}

void buildMeshData(
    std::vector<GltfDevice>& devices,
    std::vector<std::uint8_t>& binary,
    std::vector<BufferView>& bufferViews,
    std::vector<Accessor>& accessors,
    std::vector<MeshPrimitive>& meshes
)
{
    for (GltfDevice& device : devices) {
        if (!device.geometry.available || device.geometry.vertices.empty() || device.geometry.indices.empty()) {
            continue;
        }

        std::vector<float> positions;
        std::vector<float> normals;
        positions.reserve(device.geometry.vertices.size() * 3);
        normals.reserve(device.geometry.vertices.size() * 3);
        for (const RenderModelVertex& vertex : device.geometry.vertices) {
            positions.push_back(vertex.position[0]);
            positions.push_back(vertex.position[1]);
            positions.push_back(vertex.position[2]);

            normals.push_back(vertex.normal[0]);
            normals.push_back(vertex.normal[1]);
            normals.push_back(vertex.normal[2]);
        }

        const int positionView = appendFloatBufferView(binary, bufferViews, positions, kGltfArrayBuffer);
        const int normalView = appendFloatBufferView(binary, bufferViews, normals, kGltfArrayBuffer);
        const int indexView = appendUint16BufferView(
            binary,
            bufferViews,
            device.geometry.indices,
            kGltfElementArrayBuffer
        );

        const int positionAccessor = addAccessor(
            accessors,
            positionView,
            kGltfComponentFloat,
            static_cast<int>(device.geometry.vertices.size()),
            "VEC3",
            minPositionBounds(device.geometry),
            maxPositionBounds(device.geometry)
        );
        const int normalAccessor = addAccessor(
            accessors,
            normalView,
            kGltfComponentFloat,
            static_cast<int>(device.geometry.vertices.size()),
            "VEC3"
        );
        const int indexAccessor = addAccessor(
            accessors,
            indexView,
            kGltfComponentUnsignedShort,
            static_cast<int>(device.geometry.indices.size()),
            "SCALAR"
        );

        device.meshIndex = static_cast<int>(meshes.size());
        meshes.push_back({
            device.nodeName + "_Mesh",
            positionAccessor,
            normalAccessor,
            indexAccessor,
        });
    }
}

std::string makeGltfJson(
    const RecordingSession& session,
    const std::vector<GltfDevice>& devices,
    const std::vector<MeshPrimitive>& meshes,
    const std::vector<BufferView>& bufferViews,
    const std::vector<Accessor>& accessors,
    const std::vector<AnimationTarget>& animationTargets,
    const std::size_t binaryByteLength,
    const std::string& bufferUri
)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(9);
    out << "{\n";
    out << "  \"asset\": {\n";
    out << "    \"version\": \"2.0\",\n";
    out << "    \"generator\": \"OpenVR Tracker Recorder\"\n";
    out << "  },\n";
    out << "  \"scene\": 0,\n";
    out << "  \"scenes\": [\n";
    out << "    { \"name\": \"Recorded Motion\", \"nodes\": [0] }\n";
    out << "  ],\n";
    out << "  \"nodes\": [\n";
    out << "    { \"name\": \"OpenVR_Root\"";
    if (!devices.empty()) {
        out << ", \"children\": [";
        for (std::size_t i = 0; i < devices.size(); ++i) {
            if (i != 0) {
                out << ",";
            }
            out << devices[i].nodeIndex;
        }
        out << "]";
    }
    out << " }";
    for (const GltfDevice& device : devices) {
        const GltfKey defaultKey = device.keys.empty() ? GltfKey{} : device.keys.front();
        out << ",\n";
        out << "    { \"name\": \"" << escapeJsonString(device.nodeName) << "\", ";
        out << "\"translation\": ";
        writeJsonFloatArray(out, defaultKey.translation);
        out << ", \"rotation\": ";
        writeJsonFloatArray(out, defaultKey.rotation);
        if (device.meshIndex >= 0) {
            out << ", \"mesh\": " << device.meshIndex;
        }
        out << ", \"extras\": {";
        out << "\"runtimeIndex\": " << device.device.runtimeIndex;
        out << ", \"serial\": \"" << escapeJsonString(device.device.serial) << "\"";
        out << ", \"deviceClass\": \"" << escapeJsonString(toString(device.device.deviceClass)) << "\"";
        if (!device.device.role.empty()) {
            out << ", \"role\": \"" << escapeJsonString(device.device.role) << "\"";
        }
        if (!device.device.modelName.empty()) {
            out << ", \"modelName\": \"" << escapeJsonString(device.device.modelName) << "\"";
        }
        out << "} }";
    }
    out << "\n";
    out << "  ],\n";
    if (!meshes.empty()) {
        out << "  \"meshes\": [\n";
        for (std::size_t i = 0; i < meshes.size(); ++i) {
            const MeshPrimitive& mesh = meshes[i];
            if (i != 0) {
                out << ",\n";
            }
            out << "    { \"name\": \"" << escapeJsonString(mesh.name) << "\", \"primitives\": [";
            out << "{ \"attributes\": { \"POSITION\": " << mesh.positionAccessor << ", \"NORMAL\": "
                << mesh.normalAccessor << " }, \"indices\": " << mesh.indexAccessor
                << ", \"mode\": 4, \"material\": 0 }";
            out << "] }";
        }
        out << "\n";
        out << "  ],\n";
        out << "  \"materials\": [\n";
        out << "    { \"name\": \"OpenVR Device Default\", \"pbrMetallicRoughness\": { ";
        out << "\"baseColorFactor\": [0.800000000,0.820000000,0.860000000,1.000000000], ";
        out << "\"metallicFactor\": 0.000000000, \"roughnessFactor\": 0.650000000 } }\n";
        out << "  ],\n";
    }
    out << "  \"animations\": [\n";
    out << "    {\n";
    out << "      \"name\": \"" << escapeJsonString(session.sessionName.empty() ? session.sessionId : session.sessionName) << "\",\n";
    out << "      \"samplers\": [\n";
    int samplerIndex = 0;
    for (std::size_t targetIndex = 0; targetIndex < animationTargets.size(); ++targetIndex) {
        const AnimationTarget& target = animationTargets[targetIndex];
        if (targetIndex != 0) {
            out << ",\n";
        }
        out << "        { \"input\": " << target.timeAccessor << ", \"output\": " << target.translationAccessor
            << ", \"interpolation\": \"LINEAR\" },\n";
        out << "        { \"input\": " << target.timeAccessor << ", \"output\": " << target.rotationAccessor
            << ", \"interpolation\": \"LINEAR\" }";
        samplerIndex += 2;
    }
    (void)samplerIndex;
    out << "\n";
    out << "      ],\n";
    out << "      \"channels\": [\n";
    samplerIndex = 0;
    for (std::size_t targetIndex = 0; targetIndex < animationTargets.size(); ++targetIndex) {
        const AnimationTarget& target = animationTargets[targetIndex];
        if (targetIndex != 0) {
            out << ",\n";
        }
        out << "        { \"sampler\": " << samplerIndex << ", \"target\": { \"node\": " << target.node
            << ", \"path\": \"translation\" } },\n";
        out << "        { \"sampler\": " << (samplerIndex + 1) << ", \"target\": { \"node\": " << target.node
            << ", \"path\": \"rotation\" } }";
        samplerIndex += 2;
    }
    out << "\n";
    out << "      ]\n";
    out << "    }\n";
    out << "  ],\n";
    out << "  \"buffers\": [\n";
    out << "    { \"byteLength\": " << binaryByteLength;
    if (!bufferUri.empty()) {
        out << ", \"uri\": \"" << escapeJsonString(bufferUri) << "\"";
    }
    out << " }\n";
    out << "  ],\n";
    out << "  \"bufferViews\": [\n";
    for (std::size_t i = 0; i < bufferViews.size(); ++i) {
        const BufferView& view = bufferViews[i];
        if (i != 0) {
            out << ",\n";
        }
        out << "    { \"buffer\": 0, \"byteOffset\": " << view.byteOffset << ", \"byteLength\": " << view.byteLength;
        if (view.target != 0) {
            out << ", \"target\": " << view.target;
        }
        out << " }";
    }
    out << "\n";
    out << "  ],\n";
    out << "  \"accessors\": [\n";
    for (std::size_t i = 0; i < accessors.size(); ++i) {
        const Accessor& accessor = accessors[i];
        if (i != 0) {
            out << ",\n";
        }
        out << "    { \"bufferView\": " << accessor.bufferView << ", \"componentType\": " << accessor.componentType
            << ", \"count\": "
            << accessor.count << ", \"type\": \"" << accessor.type << "\"";
        if (!accessor.minValues.empty()) {
            out << ", \"min\": ";
            writeJsonNumberArray(out, accessor.minValues);
        }
        if (!accessor.maxValues.empty()) {
            out << ", \"max\": ";
            writeJsonNumberArray(out, accessor.maxValues);
        }
        out << " }";
    }
    out << "\n";
    out << "  ],\n";
    out << "  \"extras\": {\n";
    out << "    \"sessionId\": \"" << escapeJsonString(session.sessionId) << "\",\n";
    out << "    \"createdAtUtc\": \"" << escapeJsonString(session.createdAtUtc) << "\",\n";
    out << "    \"sourceCoordinateSystem\": \"" << escapeJsonString(session.coordinateSystem) << "\",\n";
    out << "    \"targetSampleRate\": " << formatDouble(session.targetSampleRate) << "\n";
    out << "  }\n";
    out << "}\n";
    return out.str();
}

void buildAnimationData(
    const RecordingSession& session,
    std::vector<GltfDevice>& devices,
    std::vector<std::uint8_t>& binary,
    std::vector<BufferView>& bufferViews,
    std::vector<Accessor>& accessors,
    std::vector<AnimationTarget>& animationTargets
)
{
    const double startTime = globalStartTime(devices);
    for (GltfDevice& device : devices) {
        if (device.keys.empty()) {
            continue;
        }

        std::vector<float> times;
        std::vector<float> translations;
        std::vector<float> rotations;
        times.reserve(device.keys.size());
        translations.reserve(device.keys.size() * 3);
        rotations.reserve(device.keys.size() * 4);

        float minTime = std::numeric_limits<float>::max();
        float maxTime = std::numeric_limits<float>::lowest();
        for (const GltfKey& key : device.keys) {
            const float time = static_cast<float>(std::max(0.0, key.timeSeconds - startTime));
            times.push_back(time);
            minTime = std::min(minTime, time);
            maxTime = std::max(maxTime, time);

            translations.push_back(key.translation[0]);
            translations.push_back(key.translation[1]);
            translations.push_back(key.translation[2]);

            rotations.push_back(key.rotation[0]);
            rotations.push_back(key.rotation[1]);
            rotations.push_back(key.rotation[2]);
            rotations.push_back(key.rotation[3]);
        }

        const int timeView = appendFloatBufferView(binary, bufferViews, times);
        const int translationView = appendFloatBufferView(binary, bufferViews, translations);
        const int rotationView = appendFloatBufferView(binary, bufferViews, rotations);

        const int timeAccessor = addAccessor(
            accessors,
            timeView,
            kGltfComponentFloat,
            static_cast<int>(times.size()),
            "SCALAR",
            {minTime},
            {maxTime}
        );
        const int translationAccessor = addAccessor(
            accessors,
            translationView,
            kGltfComponentFloat,
            static_cast<int>(device.keys.size()),
            "VEC3"
        );
        const int rotationAccessor = addAccessor(
            accessors,
            rotationView,
            kGltfComponentFloat,
            static_cast<int>(device.keys.size()),
            "VEC4"
        );

        animationTargets.push_back({
            device.nodeIndex,
            timeAccessor,
            translationAccessor,
            rotationAccessor,
        });
    }
    (void)session;
}

bool writeBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return out.good();
}

bool writeText(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out << text;
    return out.good();
}

std::vector<std::uint8_t> paddedStringBytes(const std::string& text, const char padding)
{
    std::vector<std::uint8_t> bytes(text.begin(), text.end());
    while ((bytes.size() % 4) != 0) {
        bytes.push_back(static_cast<std::uint8_t>(padding));
    }
    return bytes;
}

std::vector<std::uint8_t> paddedBinaryBytes(const std::vector<std::uint8_t>& input)
{
    std::vector<std::uint8_t> bytes = input;
    while ((bytes.size() % 4) != 0) {
        bytes.push_back(0);
    }
    return bytes;
}

bool writeGlb(const std::filesystem::path& path, const std::string& json, const std::vector<std::uint8_t>& binary)
{
    constexpr std::uint32_t kGlbMagic = 0x46546c67u;
    constexpr std::uint32_t kGlbVersion = 2u;
    constexpr std::uint32_t kJsonChunkType = 0x4e4f534au;
    constexpr std::uint32_t kBinChunkType = 0x004e4942u;

    const std::vector<std::uint8_t> jsonChunk = paddedStringBytes(json, ' ');
    const std::vector<std::uint8_t> binChunk = paddedBinaryBytes(binary);
    const std::uint32_t totalLength = static_cast<std::uint32_t>(12 + 8 + jsonChunk.size() + 8 + binChunk.size());

    std::vector<std::uint8_t> output;
    output.reserve(totalLength);
    appendUint32(output, kGlbMagic);
    appendUint32(output, kGlbVersion);
    appendUint32(output, totalLength);
    appendUint32(output, static_cast<std::uint32_t>(jsonChunk.size()));
    appendUint32(output, kJsonChunkType);
    output.insert(output.end(), jsonChunk.begin(), jsonChunk.end());
    appendUint32(output, static_cast<std::uint32_t>(binChunk.size()));
    appendUint32(output, kBinChunkType);
    output.insert(output.end(), binChunk.begin(), binChunk.end());

    return writeBytes(path, output);
}

std::filesystem::path siblingBinPath(const std::filesystem::path& gltfPath)
{
    std::filesystem::path binPath = gltfPath;
    binPath.replace_extension(".bin");
    return binPath;
}

} // namespace

ExportResult exportSessionToGltf(const RecordingSession& session, const GltfExportOptions& options)
{
    ExportResult result;
    result.outputPath = options.outputPath;

    if (options.outputPath.empty()) {
        result.error = "glTF output path is empty";
        return result;
    }

    BinarySessionReader reader;
    if (!reader.open(session.framesPath, session.frameIndexPath)) {
        result.error = "failed to open recorded session: " + reader.lastError();
        return result;
    }

    std::vector<GltfDevice> devices;
    devices.reserve(session.devices.size());
    int nextNodeIndex = 1;
    for (const DeviceDescriptor& descriptor : session.devices) {
        if (!options.includeTrackingReference && descriptor.deviceClass == DeviceClass::TrackingReference) {
            continue;
        }

        GltfDevice device;
        device.device = descriptor;
        device.nodeName = makeGltfSafeName(descriptor);
        device.nodeIndex = nextNodeIndex++;
        if (options.includeGeometry) {
            if (options.geometryProvider) {
                device.geometry = options.geometryProvider(descriptor);
            } else {
                device.geometry = loadSteamVRRenderModelGeometry(descriptor.renderModelName);
            }
            device.geometry.available = device.geometry.available ||
                (!device.geometry.vertices.empty() && !device.geometry.indices.empty());
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

            GltfKey key;
            key.timeSeconds = frame.timeSeconds;
            key.translation = pose.position;
            key.rotation = normalizeQuaternion(pose.rotation);
            devices[found->second].keys.push_back(key);
        }
    }

    for (GltfDevice& device : devices) {
        resampleKeys(device.keys, options.exportSampleRate);
    }

    std::vector<std::uint8_t> binary;
    std::vector<BufferView> bufferViews;
    std::vector<Accessor> accessors;
    std::vector<MeshPrimitive> meshes;
    std::vector<AnimationTarget> animationTargets;
    buildMeshData(devices, binary, bufferViews, accessors, meshes);
    buildAnimationData(session, devices, binary, bufferViews, accessors, animationTargets);

    std::error_code error;
    const std::filesystem::path parentPath = options.outputPath.parent_path();
    if (!parentPath.empty()) {
        std::filesystem::create_directories(parentPath, error);
        if (error) {
            result.error = "failed to create glTF export directory: " + error.message();
            return result;
        }
    }

    if (options.format == GltfExportFormat::Glb) {
        const std::string json = makeGltfJson(
            session,
            devices,
            meshes,
            bufferViews,
            accessors,
            animationTargets,
            binary.size(),
            ""
        );
        if (!writeGlb(options.outputPath, json, binary)) {
            result.error = "failed to write GLB output file";
            return result;
        }
    } else {
        const std::filesystem::path binPath = siblingBinPath(options.outputPath);
        const std::string binUri = binPath.filename().string();
        const std::string json = makeGltfJson(
            session,
            devices,
            meshes,
            bufferViews,
            accessors,
            animationTargets,
            binary.size(),
            binUri
        );
        if (!writeBytes(binPath, binary)) {
            result.error = "failed to write glTF binary buffer";
            return result;
        }
        if (!writeText(options.outputPath, json)) {
            result.error = "failed to write glTF JSON output file";
            return result;
        }
    }

    result.success = true;
    return result;
}

} // namespace ovtr
