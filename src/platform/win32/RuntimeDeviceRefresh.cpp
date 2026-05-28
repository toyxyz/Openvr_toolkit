#include "platform/win32/RuntimeStatusInternal.h"

#include "platform/win32/AppState.h"
#include "platform/win32/OriginActions.h"
#include "vr/IVRProvider.h"

#include <chrono>
#include <mutex>
#include <vector>

namespace ovtr::win32 {
namespace {

bool deviceListEventReceived(const std::vector<ovtr::VREvent>& events)
{
    for (const ovtr::VREvent& event : events) {
        if (event.type == ovtr::VREventType::DeviceActivated ||
            event.type == ovtr::VREventType::DeviceDeactivated ||
            event.type == ovtr::VREventType::DeviceUpdated) {
            return true;
        }
    }
    return false;
}

} // namespace

void refreshProviderAndDevices(AppWindowState& state, const bool forceDeviceEnumeration)
{
    std::lock_guard<std::mutex> providerLock(state.providerMutex);

    if (!state.provider.isInitialized()) {
        state.provider.initialize();
    }

    if (state.provider.isInitialized()) {
        std::vector<ovtr::VREvent> events;
        state.provider.pollEvents(events);

        const auto now = std::chrono::steady_clock::now();
        const bool deviceRefreshDue =
            state.lastDeviceEnumeration.time_since_epoch().count() == 0 ||
            std::chrono::duration<double>(now - state.lastDeviceEnumeration).count() >= 10.0;
        if (forceDeviceEnumeration ||
            state.devices.empty() ||
            deviceRefreshDue ||
            deviceListEventReceived(events)) {
            state.devices = state.provider.enumerateDevices();
            state.lastDeviceEnumeration = now;
        }
        ensureOriginSelectionForUi(state);
        clearMissingDeviceSelectionWithLog(state);
    } else {
        state.devices.clear();
        state.lastDeviceEnumeration = {};
        state.poses = {};
        ensureOriginSelectionForUi(state);
        clearMissingDeviceSelectionWithLog(state);
        state.providerError = state.provider.lastError();
    }
}

} // namespace ovtr::win32
