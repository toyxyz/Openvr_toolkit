#include "platform/win32/OriginState.h"

#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppOriginState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/DeviceList.h"

#include <mutex>

namespace ovtr::win32 {

bool clearMissingDeviceSelection(
    const AppRuntimeState& runtimeState,
    AppDeviceState& deviceState
)
{
    if (deviceState.selectedDeviceRuntimeIndex == kNoSelectedRuntimeIndex) {
        return false;
    }
    if (selectedListDevice(runtimeState, deviceState) != nullptr) {
        return false;
    }
    deviceState.selectedDeviceRuntimeIndex = kNoSelectedRuntimeIndex;
    return true;
}

ListDeviceSelectionChange toggleListDeviceSelectionState(
    AppDeviceState& state,
    const ovtr::DeviceDescriptor& device
)
{
    if (state.selectedDeviceRuntimeIndex == device.runtimeIndex) {
        state.selectedDeviceRuntimeIndex = kNoSelectedRuntimeIndex;
        return ListDeviceSelectionChange::Cleared;
    }

    state.selectedDeviceRuntimeIndex = device.runtimeIndex;
    return ListDeviceSelectionChange::Selected;
}

void clearOriginState(AppOriginState& state)
{
    std::lock_guard<std::mutex> lock(state.originMutex);
    state.originEnabled = false;
    state.originOffset = {0.0f, 0.0f, 0.0f};
    state.originRotationDegrees = {0.0f, 0.0f, 0.0f};
    state.originStatusMessage = "origin cleared";
}

} // namespace ovtr::win32
