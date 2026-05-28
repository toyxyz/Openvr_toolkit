#include "vr/OpenVRProvider.h"

#include "vr/OpenVRProviderDetails.h"
#include "vr/OpenVRProviderRuntime.h"

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>
#endif

#include <array>
#include <chrono>

namespace ovtr {
namespace {

std::uint64_t nowNs()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

} // namespace

bool OpenVRProvider::pollEvents(std::vector<VREvent>& outEvents)
{
    outEvents.clear();
#ifdef OVTR_HAS_OPENVR_SDK
    if (!initialized_ || !runtime_) {
        return false;
    }

    vr::IVRSystem* system = runtime_->system();
    if (system == nullptr) {
        return false;
    }

    vr::VREvent_t event{};
    while (system->PollNextEvent(&event, sizeof(event))) {
        outEvents.push_back({openvr_provider_detail::mapEventType(event.eventType), event.trackedDeviceIndex});
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
    if (!initialized_ || !runtime_) {
        return false;
    }

    vr::IVRSystem* system = runtime_->system();
    if (system == nullptr) {
        return false;
    }

    std::array<vr::TrackedDevicePose_t, vr::k_unMaxTrackedDeviceCount> poses{};
    system->GetDeviceToAbsoluteTrackingPose(
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

        const vr::ETrackedDeviceClass deviceClass = system->GetTrackedDeviceClass(index);
        if (deviceClass != vr::TrackedDeviceClass_HMD &&
            deviceClass != vr::TrackedDeviceClass_Controller &&
            deviceClass != vr::TrackedDeviceClass_GenericTracker &&
            deviceClass != vr::TrackedDeviceClass_TrackingReference) {
            continue;
        }

        outResult.poses.push_back(openvr_provider_detail::makePoseSample(index, poses[index]));
    }
    appendSkeletalPoses(outResult);
    return true;
#else
    return false;
#endif
}

} // namespace ovtr
