#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ovtr {

using DeviceId = std::uint32_t;

enum class DeviceClass {
    Invalid,
    Hmd,
    Controller,
    GenericTracker,
    TrackingReference,
    Other,
};

enum PoseFlags : std::uint32_t {
    PoseFlagDeviceConnected = 1u << 0,
    PoseFlagPoseValid = 1u << 1,
    PoseFlagRecordEnabled = 1u << 2,
    PoseFlagCalibratedPreview = 1u << 3,
};

struct DeviceDescriptor {
    DeviceId id = 0;
    std::uint32_t runtimeIndex = 0;
    std::string serial;
    std::string role;
    DeviceClass deviceClass = DeviceClass::Invalid;
    std::string modelName;
    std::string renderModelName;
    std::string manufacturerName;
    bool recordEnabled = true;
};

struct PoseSample {
    DeviceId deviceId = 0;
    std::uint32_t runtimeIndex = 0;

    std::array<float, 3> position{0.0f, 0.0f, 0.0f};
    std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};
    std::array<float, 3> velocity{0.0f, 0.0f, 0.0f};
    std::array<float, 3> angularVelocity{0.0f, 0.0f, 0.0f};

    std::uint32_t flags = 0;
};

struct FrameSample {
    std::uint64_t frameIndex = 0;
    std::uint64_t timestampNs = 0;
    double timeSeconds = 0.0;
    std::vector<PoseSample> poses;
};

struct RecordingSession {
    std::string sessionId;
    std::string sessionName;
    std::string createdAtUtc;
    std::string appVersion;

    double targetSampleRate = 90.0;
    std::string trackingUniverse = "Standing";
    std::string coordinateSystem = "OpenVR";
    std::string unit = "meter";

    std::vector<DeviceDescriptor> devices;
    std::filesystem::path framesPath;
    std::filesystem::path frameIndexPath;
};

std::string toString(DeviceClass deviceClass);

} // namespace ovtr

