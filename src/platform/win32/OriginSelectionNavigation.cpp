#include "platform/win32/OriginState.h"

#include "platform/win32/AppOriginState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/DeviceList.h"

namespace ovtr::win32 {

void ensureOriginSelection(const AppRuntimeState& runtimeState, AppOriginState& originState)
{
    if (runtimeState.devices.empty()) {
        originState.selectedOriginRuntimeIndex = kNoSelectedRuntimeIndex;
        return;
    }

    if (selectedOriginDevice(runtimeState, originState) == nullptr) {
        originState.selectedOriginRuntimeIndex = runtimeState.devices.front().runtimeIndex;
    }
}

std::string selectNextOriginDevice(const AppRuntimeState& runtimeState, AppOriginState& originState)
{
    if (runtimeState.devices.empty()) {
        originState.selectedOriginRuntimeIndex = kNoSelectedRuntimeIndex;
        originState.originStatusMessage = "no devices to select as origin";
        return originState.originStatusMessage;
    }

    const ovtr::DeviceDescriptor* current = selectedOriginDevice(runtimeState, originState);
    if (current == nullptr) {
        originState.selectedOriginRuntimeIndex = runtimeState.devices.front().runtimeIndex;
    } else {
        const auto currentIndex = static_cast<std::size_t>(current - runtimeState.devices.data());
        const std::size_t nextIndex = (currentIndex + 1) % runtimeState.devices.size();
        originState.selectedOriginRuntimeIndex = runtimeState.devices[nextIndex].runtimeIndex;
    }

    const ovtr::DeviceDescriptor* selected = selectedOriginDevice(runtimeState, originState);
    originState.originStatusMessage = selected
        ? "selected " + deviceDisplayName(*selected)
        : "origin selection unavailable";
    return originState.originStatusMessage;
}

} // namespace ovtr::win32
