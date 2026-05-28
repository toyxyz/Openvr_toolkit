#include "platform/win32/RuntimeStatusInternal.h"

#include "platform/win32/AppDebugUiState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/AppLog.h"

#include <string>

namespace ovtr::win32 {

void appendProviderStatusLogChanges(
    AppRuntimeState& runtimeState,
    AppDebugUiState& logState,
    const bool providerWasInitialized
)
{
    if (!providerWasInitialized && runtimeState.provider.isInitialized()) {
        appendDebugLog(logState, L"OpenVR provider initialized");
    }
    if (runtimeState.providerError != runtimeState.lastLoggedProviderError) {
        if (runtimeState.providerError.empty()) {
            appendDebugLog(logState, L"OpenVR provider error cleared");
        } else {
            appendDebugLog(logState, "OpenVR provider error: " + runtimeState.providerError);
        }
        runtimeState.lastLoggedProviderError = runtimeState.providerError;
    }
    if (runtimeState.devices.size() != runtimeState.lastLoggedDeviceCount) {
        appendDebugLog(logState, L"Tracked device count: " + std::to_wstring(runtimeState.devices.size()));
        runtimeState.lastLoggedDeviceCount = runtimeState.devices.size();
    }
}

} // namespace ovtr::win32
