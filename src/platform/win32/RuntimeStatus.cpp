#include "platform/win32/RuntimeStatus.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/RuntimeStatusInternal.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {

void refreshStatus(HWND hwnd, const bool forceDeviceEnumeration)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return;
    }

    if (forceDeviceEnumeration) {
        appendDebugLog(*state, L"Manual runtime/device refresh requested");
    }

    const bool providerWasInitialized = state->provider.isInitialized();
    state->status = state->runtime.queryStatus();
    state->providerError.clear();

    refreshProviderAndDevices(*state, forceDeviceEnumeration);
    appendProviderStatusLogChanges(
        static_cast<AppRuntimeState&>(*state),
        static_cast<AppDebugUiState&>(*state),
        providerWasInitialized
    );
    updateFpsCounters(static_cast<AppRuntimeState&>(*state));
    invalidateStatusPanel(hwnd);
}

} // namespace ovtr::win32
