#pragma once

#include "data/SessionTypes.h"
#include "vr/IVRProvider.h"

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>
#endif

#include <cstdint>
#include <string>

namespace ovtr::openvr_provider_detail {

#ifdef OVTR_HAS_OPENVR_SDK
DeviceClass mapDeviceClass(vr::ETrackedDeviceClass deviceClass);
VREventType mapEventType(std::uint32_t eventType);

std::string getStringProperty(
    vr::IVRSystem* system,
    vr::TrackedDeviceIndex_t index,
    vr::ETrackedDeviceProperty property
);

PoseSample makePoseSample(
    vr::TrackedDeviceIndex_t index,
    const vr::TrackedDevicePose_t& trackedPose
);

std::string initErrorMessage(vr::EVRInitError error);
#endif

} // namespace ovtr::openvr_provider_detail
