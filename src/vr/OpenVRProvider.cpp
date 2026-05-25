#include "vr/OpenVRProvider.h"

#include "math/QuaternionUtils.h"

#include <array>
#include <chrono>
#include <cmath>
#include <sstream>
#include <utility>

namespace ovtr {

namespace {

std::uint64_t nowNs()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

#ifdef OVTR_HAS_OPENVR_SDK

DeviceClass mapDeviceClass(const vr::ETrackedDeviceClass deviceClass)
{
    switch (deviceClass) {
    case vr::TrackedDeviceClass_HMD:
        return DeviceClass::Hmd;
    case vr::TrackedDeviceClass_Controller:
        return DeviceClass::Controller;
    case vr::TrackedDeviceClass_GenericTracker:
        return DeviceClass::GenericTracker;
    case vr::TrackedDeviceClass_TrackingReference:
        return DeviceClass::TrackingReference;
    case vr::TrackedDeviceClass_Invalid:
        return DeviceClass::Invalid;
    default:
        return DeviceClass::Other;
    }
}

VREventType mapEventType(const std::uint32_t eventType)
{
    switch (eventType) {
    case vr::VREvent_TrackedDeviceActivated:
        return VREventType::DeviceActivated;
    case vr::VREvent_TrackedDeviceDeactivated:
        return VREventType::DeviceDeactivated;
    case vr::VREvent_TrackedDeviceUpdated:
        return VREventType::DeviceUpdated;
    case vr::VREvent_Quit:
        return VREventType::Quit;
    default:
        return VREventType::Other;
    }
}

std::string getStringProperty(
    vr::IVRSystem* system,
    const vr::TrackedDeviceIndex_t index,
    const vr::ETrackedDeviceProperty property
)
{
    if (system == nullptr) {
        return {};
    }

    vr::ETrackedPropertyError error = vr::TrackedProp_Success;
    const std::uint32_t required = system->GetStringTrackedDeviceProperty(index, property, nullptr, 0, &error);
    if (required == 0 || (error != vr::TrackedProp_Success && error != vr::TrackedProp_BufferTooSmall)) {
        return {};
    }

    std::string value(required, '\0');
    error = vr::TrackedProp_Success;
    system->GetStringTrackedDeviceProperty(index, property, value.data(), required, &error);
    if (error != vr::TrackedProp_Success) {
        return {};
    }

    if (!value.empty() && value.back() == '\0') {
        value.pop_back();
    }
    return value;
}

std::array<float, 4> quaternionFromOpenVRMatrix(const vr::HmdMatrix34_t& matrix)
{
    const float m00 = matrix.m[0][0];
    const float m01 = matrix.m[0][1];
    const float m02 = matrix.m[0][2];
    const float m10 = matrix.m[1][0];
    const float m11 = matrix.m[1][1];
    const float m12 = matrix.m[1][2];
    const float m20 = matrix.m[2][0];
    const float m21 = matrix.m[2][1];
    const float m22 = matrix.m[2][2];

    std::array<float, 4> q{};
    const float trace = m00 + m11 + m22;
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        q[3] = 0.25f * s;
        q[0] = (m21 - m12) / s;
        q[1] = (m02 - m20) / s;
        q[2] = (m10 - m01) / s;
    } else if (m00 > m11 && m00 > m22) {
        const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
        q[3] = (m21 - m12) / s;
        q[0] = 0.25f * s;
        q[1] = (m01 + m10) / s;
        q[2] = (m02 + m20) / s;
    } else if (m11 > m22) {
        const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
        q[3] = (m02 - m20) / s;
        q[0] = (m01 + m10) / s;
        q[1] = 0.25f * s;
        q[2] = (m12 + m21) / s;
    } else {
        const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
        q[3] = (m10 - m01) / s;
        q[0] = (m02 + m20) / s;
        q[1] = (m12 + m21) / s;
        q[2] = 0.25f * s;
    }

    return normalizeQuaternion(q);
}

PoseSample makePoseSample(
    const vr::TrackedDeviceIndex_t index,
    const vr::TrackedDevicePose_t& trackedPose
)
{
    PoseSample pose;
    pose.deviceId = static_cast<DeviceId>(index + 1);
    pose.runtimeIndex = index;

    const vr::HmdMatrix34_t& matrix = trackedPose.mDeviceToAbsoluteTracking;
    pose.position = {matrix.m[0][3], matrix.m[1][3], matrix.m[2][3]};
    pose.rotation = quaternionFromOpenVRMatrix(matrix);
    pose.velocity = {
        trackedPose.vVelocity.v[0],
        trackedPose.vVelocity.v[1],
        trackedPose.vVelocity.v[2],
    };
    pose.angularVelocity = {
        trackedPose.vAngularVelocity.v[0],
        trackedPose.vAngularVelocity.v[1],
        trackedPose.vAngularVelocity.v[2],
    };

    pose.flags = 0;
    if (trackedPose.bDeviceIsConnected) {
        pose.flags |= PoseFlagDeviceConnected;
    }
    if (trackedPose.bPoseIsValid) {
        pose.flags |= PoseFlagPoseValid;
    }
    pose.flags |= PoseFlagRecordEnabled;
    return pose;
}

std::string initErrorMessage(const vr::EVRInitError error)
{
    std::ostringstream stream;
    stream << vr::VR_GetVRInitErrorAsSymbol(error) << ": "
           << vr::VR_GetVRInitErrorAsEnglishDescription(error);
    return stream.str();
}

#endif

} // namespace

bool OpenVRProvider::initialize()
{
#ifdef OVTR_HAS_OPENVR_SDK
    lastError_.clear();
    if (initialized_) {
        return true;
    }

    if (!vr::VR_IsRuntimeInstalled()) {
        state_ = VRConnectionState::RuntimeNotInstalled;
        lastError_ = "OpenVR runtime is not installed";
        return false;
    }

    if (!vr::VR_IsHmdPresent()) {
        state_ = VRConnectionState::HmdNotPresent;
    }

    vr::EVRInitError error = vr::VRInitError_None;
    system_ = vr::VR_Init(&error, vr::VRApplication_Background);
    if (error != vr::VRInitError_None || system_ == nullptr) {
        system_ = nullptr;
        initialized_ = false;
        state_ = VRConnectionState::InitFailed;
        lastError_ = initErrorMessage(error);
        return false;
    }

    initialized_ = true;
    state_ = VRConnectionState::Connected;
    return true;
#else
    initialized_ = false;
    state_ = VRConnectionState::InitFailed;
    lastError_ = "OpenVR SDK integration has not been enabled yet";
    return false;
#endif
}

void OpenVRProvider::shutdown()
{
#ifdef OVTR_HAS_OPENVR_SDK
    if (initialized_) {
        vr::VR_Shutdown();
    }
    system_ = nullptr;
#endif
    initialized_ = false;
    state_ = VRConnectionState::SteamVRNotRunning;
}

bool OpenVRProvider::isInitialized() const
{
    return initialized_;
}

VRConnectionState OpenVRProvider::connectionState() const
{
    return state_;
}

std::string OpenVRProvider::lastError() const
{
    return lastError_;
}

bool OpenVRProvider::pollEvents(std::vector<VREvent>& outEvents)
{
    outEvents.clear();
#ifdef OVTR_HAS_OPENVR_SDK
    if (!initialized_ || system_ == nullptr) {
        return false;
    }

    vr::VREvent_t event{};
    while (system_->PollNextEvent(&event, sizeof(event))) {
        outEvents.push_back({mapEventType(event.eventType), event.trackedDeviceIndex});
    }
    return true;
#else
    return false;
#endif
}

bool OpenVRProvider::pollPoses(PosePollResult& outResult)
{
    outResult = {};
#ifdef OVTR_HAS_OPENVR_SDK
    if (!initialized_ || system_ == nullptr) {
        return false;
    }

    std::array<vr::TrackedDevicePose_t, vr::k_unMaxTrackedDeviceCount> poses{};
    system_->GetDeviceToAbsoluteTrackingPose(
        vr::TrackingUniverseStanding,
        0.0f,
        poses.data(),
        static_cast<std::uint32_t>(poses.size())
    );

    outResult.timestampNs = nowNs();
    for (vr::TrackedDeviceIndex_t index = 0; index < poses.size(); ++index) {
        if (!poses[index].bDeviceIsConnected) {
            continue;
        }

        const vr::ETrackedDeviceClass deviceClass = system_->GetTrackedDeviceClass(index);
        if (deviceClass != vr::TrackedDeviceClass_HMD &&
            deviceClass != vr::TrackedDeviceClass_Controller &&
            deviceClass != vr::TrackedDeviceClass_GenericTracker &&
            deviceClass != vr::TrackedDeviceClass_TrackingReference) {
            continue;
        }

        outResult.poses.push_back(makePoseSample(index, poses[index]));
    }
    return true;
#else
    return false;
#endif
}

std::vector<DeviceDescriptor> OpenVRProvider::enumerateDevices() const
{
    std::vector<DeviceDescriptor> devices;
#ifdef OVTR_HAS_OPENVR_SDK
    if (!initialized_ || system_ == nullptr) {
        return devices;
    }

    for (vr::TrackedDeviceIndex_t index = 0; index < vr::k_unMaxTrackedDeviceCount; ++index) {
        if (!system_->IsTrackedDeviceConnected(index)) {
            continue;
        }

        const vr::ETrackedDeviceClass deviceClass = system_->GetTrackedDeviceClass(index);
        if (deviceClass == vr::TrackedDeviceClass_Invalid) {
            continue;
        }

        DeviceDescriptor descriptor;
        descriptor.id = static_cast<DeviceId>(index + 1);
        descriptor.runtimeIndex = index;
        descriptor.deviceClass = mapDeviceClass(deviceClass);
        descriptor.serial = getStringProperty(system_, index, vr::Prop_SerialNumber_String);
        descriptor.modelName = getStringProperty(system_, index, vr::Prop_ModelNumber_String);
        descriptor.renderModelName = getStringProperty(system_, index, vr::Prop_RenderModelName_String);
        descriptor.manufacturerName = getStringProperty(system_, index, vr::Prop_ManufacturerName_String);
        descriptor.recordEnabled =
            descriptor.deviceClass == DeviceClass::Hmd ||
            descriptor.deviceClass == DeviceClass::Controller ||
            descriptor.deviceClass == DeviceClass::GenericTracker;
        devices.push_back(std::move(descriptor));
    }
#endif
    return devices;
}

} // namespace ovtr
