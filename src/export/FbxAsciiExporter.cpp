#include "export/FbxAsciiExporter.h"

#include "math/QuaternionUtils.h"
#include "recording/BinarySessionReader.h"

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ovtr {

namespace {

constexpr std::int64_t kFbxTicksPerSecond = 46'186'158'000LL;
constexpr double kPi = 3.14159265358979323846;
constexpr double kRadiansToDegrees = 57.295779513082320876;

struct FbxVertex {
    std::array<double, 3> position{0.0, 0.0, 0.0};
    std::array<double, 3> normal{0.0, 1.0, 0.0};
};

struct FbxGeometry {
    bool available = false;
    std::vector<FbxVertex> vertices;
    std::vector<std::uint16_t> indices;
};

struct PoseKey {
    double timeSeconds = 0.0;
    std::array<double, 3> translation{0.0, 0.0, 0.0};
    std::array<double, 3> rotationDegrees{0.0, 0.0, 0.0};
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
    FbxGeometry geometry;
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

double unrollAngleNearPrevious(double angle, const double previous)
{
    while (angle - previous > 180.0) {
        angle -= 360.0;
    }
    while (angle - previous < -180.0) {
        angle += 360.0;
    }
    return angle;
}

void applyEulerUnroll(std::vector<PoseKey>& keys)
{
    if (keys.size() < 2) {
        return;
    }

    for (std::size_t keyIndex = 1; keyIndex < keys.size(); ++keyIndex) {
        for (std::size_t axis = 0; axis < keys[keyIndex].rotationDegrees.size(); ++axis) {
            keys[keyIndex].rotationDegrees[axis] =
                unrollAngleNearPrevious(keys[keyIndex].rotationDegrees[axis], keys[keyIndex - 1].rotationDegrees[axis]);
        }
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

std::vector<double> collectGeometryVertices(const FbxGeometry& geometry)
{
    std::vector<double> values;
    values.reserve(geometry.vertices.size() * 3);
    for (const FbxVertex& vertex : geometry.vertices) {
        values.push_back(vertex.position[0]);
        values.push_back(vertex.position[1]);
        values.push_back(vertex.position[2]);
    }
    return values;
}

std::vector<double> collectGeometryNormals(const FbxGeometry& geometry)
{
    std::vector<double> values;
    values.reserve(geometry.indices.size() * 3);
    for (const std::uint16_t index : geometry.indices) {
        if (index >= geometry.vertices.size()) {
            continue;
        }
        const FbxVertex& vertex = geometry.vertices[index];
        values.push_back(vertex.normal[0]);
        values.push_back(vertex.normal[1]);
        values.push_back(vertex.normal[2]);
    }
    return values;
}

std::vector<std::int32_t> collectPolygonVertexIndex(const FbxGeometry& geometry)
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

void convertGeometry(FbxGeometry& geometry, const FbxCoordinatePolicy policy)
{
    if (policy != FbxCoordinatePolicy::Blender) {
        return;
    }

    for (FbxVertex& vertex : geometry.vertices) {
        vertex.position = convertVector(vertex.position, policy);
        vertex.normal = convertVector(vertex.normal, policy);
    }
}

FbxGeometry loadSteamVRGeometry(const std::string& renderModelName, const FbxCoordinatePolicy policy)
{
    FbxGeometry geometry;
    if (renderModelName.empty()) {
        return geometry;
    }

#ifdef OVTR_HAS_OPENVR_SDK
    vr::IVRRenderModels* renderModels = vr::VRRenderModels();
    if (renderModels == nullptr) {
        return geometry;
    }

    vr::RenderModel_t* openvrModel = nullptr;
    vr::EVRRenderModelError error = vr::VRRenderModelError_Loading;
    for (int attempt = 0; attempt < 50; ++attempt) {
        error = renderModels->LoadRenderModel_Async(renderModelName.c_str(), &openvrModel);
        if (error != vr::VRRenderModelError_Loading) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (error != vr::VRRenderModelError_None || openvrModel == nullptr) {
        return geometry;
    }

    geometry.vertices.reserve(openvrModel->unVertexCount);
    for (std::uint32_t i = 0; i < openvrModel->unVertexCount; ++i) {
        const vr::RenderModel_Vertex_t& source = openvrModel->rVertexData[i];
        FbxVertex vertex;
        vertex.position = {
            static_cast<double>(source.vPosition.v[0]),
            static_cast<double>(source.vPosition.v[1]),
            static_cast<double>(source.vPosition.v[2]),
        };
        vertex.normal = {
            static_cast<double>(source.vNormal.v[0]),
            static_cast<double>(source.vNormal.v[1]),
            static_cast<double>(source.vNormal.v[2]),
        };
        geometry.vertices.push_back(vertex);
    }

    const std::uint32_t indexCount = openvrModel->unTriangleCount * 3;
    geometry.indices.assign(openvrModel->rIndexData, openvrModel->rIndexData + indexCount);
    renderModels->FreeRenderModel(openvrModel);
    geometry.available = !geometry.vertices.empty() && !geometry.indices.empty();
#else
    (void)renderModelName;
#endif
    convertGeometry(geometry, policy);
    return geometry;
}

void writeHeader(std::ostream& out, const FbxCoordinatePolicy policy)
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
    const std::array<float, 4> q = normalizeQuaternion(quaternion);
    const double x = q[0];
    const double y = q[1];
    const double z = q[2];
    const double w = q[3];

    const double sinPitchRaw = 2.0 * (w * y - z * x);
    double roll = 0.0;
    double pitch = 0.0;
    double yaw = 0.0;

    if (std::fabs(sinPitchRaw) >= 1.0 - 1.0e-8) {
        pitch = std::copysign(kPi * 0.5, sinPitchRaw);
    } else {
        const double sinRollCosPitch = 2.0 * (w * x + y * z);
        const double cosRollCosPitch = 1.0 - 2.0 * (x * x + y * y);
        roll = std::atan2(sinRollCosPitch, cosRollCosPitch);

        pitch = std::asin(std::clamp(sinPitchRaw, -1.0, 1.0));

        const double sinYawCosPitch = 2.0 * (w * z + x * y);
        const double cosYawCosPitch = 1.0 - 2.0 * (y * y + z * z);
        yaw = std::atan2(sinYawCosPitch, cosYawCosPitch);
    }

    return {roll * kRadiansToDegrees, pitch * kRadiansToDegrees, yaw * kRadiansToDegrees};
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
            device.geometry = loadSteamVRGeometry(descriptor.renderModelName, options.coordinatePolicy);
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
            key.rotationDegrees = quaternionToEulerXyzDegrees(convertQuaternion(pose.rotation, options.coordinatePolicy));
            devices[found->second].keys.push_back(key);
        }
    }

    if (options.applyEulerUnroll) {
        for (DeviceExport& device : devices) {
            applyEulerUnroll(device.keys);
        }
    }

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

    writeHeader(out, options.coordinatePolicy);
    out << "Objects:  {\n";
    writeRootModel(out, rootModelId);
    for (const DeviceExport& device : devices) {
        writeGeometry(out, device);
        writeModel(out, device);
    }
    out << "    AnimationStack: " << stackId << ", \"AnimStack::Recorded Motion\", \"\" {\n";
    out << "    }\n";
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
