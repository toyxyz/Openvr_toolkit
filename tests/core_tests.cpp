#include "data/SessionTypes.h"
#include "export/FbxAsciiExporter.h"
#include "math/QuaternionUtils.h"
#include "recording/BinarySessionReader.h"
#include "recording/BinarySessionWriter.h"
#include "recording/RecordingController.h"
#include "recording/SamplingScheduler.h"
#include "recording/SessionManifest.h"
#include "vr/MockVRProvider.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

constexpr double kTestPi = 3.14159265358979323846;
constexpr double kTestDegreesToRadians = kTestPi / 180.0;

void require(const bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::array<double, 9> multiplyMatrix3x3(const std::array<double, 9>& left, const std::array<double, 9>& right)
{
    std::array<double, 9> result{};
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            result[static_cast<std::size_t>(row * 3 + column)] =
                left[static_cast<std::size_t>(row * 3 + 0)] * right[static_cast<std::size_t>(0 * 3 + column)] +
                left[static_cast<std::size_t>(row * 3 + 1)] * right[static_cast<std::size_t>(1 * 3 + column)] +
                left[static_cast<std::size_t>(row * 3 + 2)] * right[static_cast<std::size_t>(2 * 3 + column)];
        }
    }
    return result;
}

std::array<double, 9> matrixFromEulerXyzDegrees(const std::array<double, 3>& eulerDegrees)
{
    const double x = eulerDegrees[0] * kTestDegreesToRadians;
    const double y = eulerDegrees[1] * kTestDegreesToRadians;
    const double z = eulerDegrees[2] * kTestDegreesToRadians;
    const double cx = std::cos(x);
    const double sx = std::sin(x);
    const double cy = std::cos(y);
    const double sy = std::sin(y);
    const double cz = std::cos(z);
    const double sz = std::sin(z);

    const std::array<double, 9> rotateX{
        1.0, 0.0, 0.0,
        0.0, cx, -sx,
        0.0, sx, cx,
    };
    const std::array<double, 9> rotateY{
        cy, 0.0, sy,
        0.0, 1.0, 0.0,
        -sy, 0.0, cy,
    };
    const std::array<double, 9> rotateZ{
        cz, -sz, 0.0,
        sz, cz, 0.0,
        0.0, 0.0, 1.0,
    };

    return multiplyMatrix3x3(multiplyMatrix3x3(rotateZ, rotateY), rotateX);
}

std::array<double, 9> matrixFromQuaternion(const std::array<float, 4>& quaternion)
{
    const std::array<float, 4> q = ovtr::normalizeQuaternion(quaternion);
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

double maxMatrixDelta(const std::array<double, 9>& left, const std::array<double, 9>& right)
{
    double delta = 0.0;
    for (std::size_t i = 0; i < left.size(); ++i) {
        delta = std::max(delta, std::fabs(left[i] - right[i]));
    }
    return delta;
}

void testSamplingScheduler()
{
    using clock = std::chrono::steady_clock;

    ovtr::SamplingScheduler scheduler(10.0);
    const auto start = clock::time_point{};
    scheduler.reset(start);

    require(scheduler.shouldSample(start), "scheduler should sample at start");
    const ovtr::SampleTiming first = scheduler.markSampled(start);
    require(first.frameIndex == 0, "first frame index should be zero");
    require(first.droppedFrames == 0, "first sample should not drop frames");
    require(!scheduler.shouldSample(start + std::chrono::milliseconds(50)), "scheduler sampled too early");
    require(scheduler.shouldSample(start + std::chrono::milliseconds(100)), "scheduler missed due sample");

    const ovtr::SampleTiming late = scheduler.markSampled(start + std::chrono::milliseconds(350));
    require(late.frameIndex == 1, "late sample frame index mismatch");
    require(late.droppedFrames == 2, "late sample should report two dropped frames");
    require(scheduler.droppedFrameCount() == 2, "scheduler cumulative dropped frame count mismatch");
}

ovtr::FrameSample makeTestFrame(const std::uint64_t frameIndex)
{
    ovtr::FrameSample frame;
    frame.frameIndex = frameIndex;
    frame.timestampNs = frameIndex * 11'111'111;
    frame.timeSeconds = static_cast<double>(frame.timestampNs) / 1'000'000'000.0;

    ovtr::PoseSample pose;
    pose.deviceId = 1;
    pose.runtimeIndex = 3;
    pose.position = {1.0f + static_cast<float>(frameIndex), 2.0f, 3.0f};
    pose.rotation = ovtr::normalizeQuaternion({0.0f, 0.5f, 0.0f, 0.5f});
    pose.velocity = {0.1f, 0.2f, 0.3f};
    pose.angularVelocity = {0.01f, 0.02f, 0.03f};
    pose.flags = ovtr::PoseFlagDeviceConnected | ovtr::PoseFlagPoseValid | ovtr::PoseFlagRecordEnabled;
    frame.poses.push_back(pose);

    return frame;
}

void testBinarySessionRoundTrip()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_core_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    const std::filesystem::path framesPath = testDir / "frames.bin";
    const std::filesystem::path indexPath = testDir / "frame_index.bin";

    ovtr::BinarySessionWriter writer;
    require(writer.open(framesPath, indexPath), "writer open failed: " + writer.lastError());
    require(writer.appendFrame(makeTestFrame(0)), "append frame 0 failed: " + writer.lastError());
    require(writer.appendFrame(makeTestFrame(1)), "append frame 1 failed: " + writer.lastError());
    require(writer.frameCount() == 2, "writer frame count mismatch");
    writer.close();

    ovtr::BinarySessionReader reader;
    require(reader.open(framesPath, indexPath), "reader open failed: " + reader.lastError());
    require(reader.frameCount() == 2, "reader frame count mismatch");

    ovtr::FrameSample frame;
    require(reader.readFrame(1, frame), "reader failed to read frame 1: " + reader.lastError());
    require(frame.frameIndex == 1, "read frame index mismatch");
    require(frame.poses.size() == 1, "read pose count mismatch");
    require(frame.poses[0].deviceId == 1, "read pose device id mismatch");
    require(std::fabs(frame.poses[0].position[0] - 2.0f) < 0.0001f, "read pose position mismatch");
    require(ovtr::isNearlyUnitQuaternion(frame.poses[0].rotation), "read quaternion should be normalized");

    reader.close();
    std::filesystem::remove_all(testDir, ignored);
}

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

std::string readTextFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::string findFbxObjectSection(const std::string& fbx, const std::string& marker)
{
    const std::size_t start = fbx.find(marker);
    if (start == std::string::npos) {
        return {};
    }

    const std::size_t nextCurve = fbx.find("    AnimationCurve:", start + marker.size());
    if (nextCurve == std::string::npos) {
        return fbx.substr(start);
    }
    return fbx.substr(start, nextCurve - start);
}

void testFbxQuaternionToEuler()
{
    const auto identity = ovtr::quaternionToEulerXyzDegrees({0.0f, 0.0f, 0.0f, 1.0f});
    require(std::fabs(identity[0]) < 0.001, "identity quaternion roll mismatch");
    require(std::fabs(identity[1]) < 0.001, "identity quaternion pitch mismatch");
    require(std::fabs(identity[2]) < 0.001, "identity quaternion yaw mismatch");

    const float halfSqrt = std::sqrt(0.5f);
    const auto x90 = ovtr::quaternionToEulerXyzDegrees({halfSqrt, 0.0f, 0.0f, halfSqrt});
    const auto y90 = ovtr::quaternionToEulerXyzDegrees({0.0f, halfSqrt, 0.0f, halfSqrt});
    const auto z90 = ovtr::quaternionToEulerXyzDegrees({0.0f, 0.0f, halfSqrt, halfSqrt});
    require(std::fabs(x90[0] - 90.0) < 0.01, "x 90 quaternion euler mismatch");
    require(std::fabs(y90[1] - 90.0) < 0.01, "y 90 quaternion euler mismatch");
    require(std::fabs(z90[2] - 90.0) < 0.01, "z 90 quaternion euler mismatch");

    const auto mixedQuaternion = ovtr::normalizeQuaternion({0.182574f, -0.365148f, 0.547723f, 0.730297f});
    const auto mixedEuler = ovtr::quaternionToEulerXyzDegrees(mixedQuaternion);
    const auto sourceMatrix = matrixFromQuaternion(mixedQuaternion);
    const auto reconstructedMatrix = matrixFromEulerXyzDegrees(mixedEuler);
    require(
        maxMatrixDelta(sourceMatrix, reconstructedMatrix) < 0.00001,
        "mixed-axis quaternion should round-trip through FBX XYZ euler"
    );
}

void testFbxSafeName()
{
    ovtr::DeviceDescriptor device;
    device.runtimeIndex = 7;
    device.deviceClass = ovtr::DeviceClass::GenericTracker;
    device.serial = "LHR-ABC 123/!";
    require(ovtr::makeFbxSafeName(device) == "Tracker_LHR_ABC_123", "FBX safe name mismatch");

    device.serial.clear();
    require(ovtr::makeFbxSafeName(device) == "Tracker_Device_7", "FBX fallback safe name mismatch");
}

void testFbxAsciiExport()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_fbx_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    const std::filesystem::path framesPath = testDir / "frames.bin";
    const std::filesystem::path indexPath = testDir / "frame_index.bin";
    const std::filesystem::path fbxPath = testDir / "export.fbx";

    ovtr::FrameSample frame0 = makeTestFrame(0);
    ovtr::FrameSample frame1 = makeTestFrame(1);
    const float halfSqrt = std::sqrt(0.5f);
    frame0.poses[0].rotation = ovtr::normalizeQuaternion({0.0f, 0.0f, halfSqrt, halfSqrt});
    frame1.poses[0].rotation = ovtr::normalizeQuaternion({0.0f, 0.0f, halfSqrt, halfSqrt});
    ovtr::PoseSample invalidPose = frame1.poses[0];
    invalidPose.runtimeIndex = 4;
    invalidPose.deviceId = 2;
    invalidPose.flags = ovtr::PoseFlagDeviceConnected | ovtr::PoseFlagRecordEnabled;
    frame1.poses.push_back(invalidPose);

    ovtr::BinarySessionWriter writer;
    require(writer.open(framesPath, indexPath), "FBX test writer open failed: " + writer.lastError());
    require(writer.appendFrame(frame0), "FBX test append frame 0 failed: " + writer.lastError());
    require(writer.appendFrame(frame1), "FBX test append frame 1 failed: " + writer.lastError());
    writer.close();

    ovtr::DeviceDescriptor tracker;
    tracker.id = 1;
    tracker.runtimeIndex = 3;
    tracker.serial = "LHR-TEST";
    tracker.deviceClass = ovtr::DeviceClass::GenericTracker;

    ovtr::DeviceDescriptor invalidTracker;
    invalidTracker.id = 2;
    invalidTracker.runtimeIndex = 4;
    invalidTracker.serial = "LHR-INVALID";
    invalidTracker.deviceClass = ovtr::DeviceClass::GenericTracker;

    ovtr::RecordingSession session;
    session.sessionId = "fbx-test";
    session.sessionName = "FBX Test";
    session.framesPath = framesPath;
    session.frameIndexPath = indexPath;
    session.devices = {tracker, invalidTracker};

    ovtr::FbxExportOptions options;
    options.outputPath = fbxPath;
    options.includeGeometry = false;
    const ovtr::ExportResult result = ovtr::exportSessionToFbxAscii(session, options);
    require(result.success, "FBX export failed: " + result.error);

    const std::string fbx = readTextFile(fbxPath);
    require(fbx.find("FBXVersion: 7400") != std::string::npos, "FBX version missing");
    require(fbx.find("P: \"UpAxis\", \"int\", \"Integer\", \"\",2") != std::string::npos, "FBX Blender-pass-through up axis missing");
    require(fbx.find("P: \"FrontAxis\", \"int\", \"Integer\", \"\",1") != std::string::npos, "FBX Blender-pass-through front axis missing");
    require(fbx.find("P: \"FrontAxisSign\", \"int\", \"Integer\", \"\",-1") != std::string::npos, "FBX Blender-pass-through front sign missing");
    require(fbx.find("P: \"CoordAxis\", \"int\", \"Integer\", \"\",0") != std::string::npos, "FBX Blender-pass-through coord axis missing");
    require(fbx.find("Model::OpenVR_Root") != std::string::npos, "FBX root model missing");
    require(fbx.find("Model::Tracker_LHR_TEST") != std::string::npos, "FBX tracker model missing");
    require(fbx.find("AnimCurveNode::Tracker_LHR_TEST_T\"") != std::string::npos, "FBX translation curve node missing");
    require(fbx.find("AnimCurveNode::Tracker_LHR_TEST_R\"") != std::string::npos, "FBX rotation curve node missing");
    require(fbx.find("P: \"RotationOrder\", \"enum\", \"\", \"\",0") != std::string::npos, "FBX XYZ rotation order missing");
    require(fbx.find("AnimCurveNode::Tracker_LHR_TEST_T_X\"") == std::string::npos, "FBX should not create per-axis translation curve nodes");
    require(fbx.find("AnimCurve::Tracker_LHR_TEST_T_X") != std::string::npos, "FBX translation curve missing");
    require(fbx.find("AnimCurve::Tracker_LHR_TEST_T_Y") != std::string::npos, "FBX translation Y curve missing");
    require(fbx.find("AnimCurve::Tracker_LHR_TEST_T_Z") != std::string::npos, "FBX translation Z curve missing");
    const std::string translationYCurve = findFbxObjectSection(fbx, "AnimCurve::Tracker_LHR_TEST_T_Y");
    const std::string translationZCurve = findFbxObjectSection(fbx, "AnimCurve::Tracker_LHR_TEST_T_Z");
    require(translationYCurve.find("-3.000000000") != std::string::npos, "FBX Y translation should use Blender front/back");
    require(translationZCurve.find("2.000000000") != std::string::npos, "FBX Z translation should use Blender height");
    require(fbx.find("AnimCurve::Tracker_LHR_TEST_R_Y") != std::string::npos, "FBX rotation Y curve missing");
    require(fbx.find("AnimCurve::Tracker_LHR_TEST_R_Z") != std::string::npos, "FBX rotation Z curve missing");
    const std::string rotationYCurve = findFbxObjectSection(fbx, "AnimCurve::Tracker_LHR_TEST_R_Y");
    const std::string rotationZCurve = findFbxObjectSection(fbx, "AnimCurve::Tracker_LHR_TEST_R_Z");
    require(rotationYCurve.find("-90.0000") != std::string::npos, "FBX OpenVR Z rotation should become Blender R_Y");
    require(rotationZCurve.find("a: 0.000000000,0.000000000") != std::string::npos, "FBX Z curve should not receive OpenVR Z rotation");
    require(fbx.find("AnimCurve::Tracker_LHR_INVALID_T_X") != std::string::npos, "FBX invalid tracker curve missing");
    require(fbx.find("KeyValueFloat: *0") != std::string::npos, "invalid pose should not create key values");
    require(fbx.find("C: \"OO\",1000000,0") == std::string::npos, "FBX device should not be connected directly to the scene");
    require(fbx.find("C: \"OO\",1000000,") != std::string::npos, "FBX device should be connected to a parent root");

    std::filesystem::remove_all(testDir, ignored);
}

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

void testMockVRProvider()
{
    ovtr::MockVRProvider provider;
    require(provider.initialize(), "mock provider failed to initialize");
    require(provider.isInitialized(), "mock provider should be initialized");

    ovtr::DeviceDescriptor tracker;
    tracker.id = 1;
    tracker.serial = "LHR-TEST";
    tracker.deviceClass = ovtr::DeviceClass::GenericTracker;
    provider.setDevices({tracker});

    const auto devices = provider.enumerateDevices();
    require(devices.size() == 1, "mock provider device count mismatch");
    require(devices[0].serial == "LHR-TEST", "mock provider serial mismatch");
}

} // namespace

int main()
{
    try {
        testSamplingScheduler();
        testBinarySessionRoundTrip();
        testManifestWriter();
        testFbxQuaternionToEuler();
        testFbxSafeName();
        testFbxAsciiExport();
        testRecordingController();
        testMockVRProvider();
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }

    std::cout << "core tests passed\n";
    return 0;
}
