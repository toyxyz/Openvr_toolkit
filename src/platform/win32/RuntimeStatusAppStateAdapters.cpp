#include "platform/win32/RuntimeStatusInternal.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

void appendProviderStatusLogChanges(AppWindowState& state, const bool providerWasInitialized)
{
    appendProviderStatusLogChanges(
        static_cast<AppRuntimeState&>(state),
        static_cast<AppDebugUiState&>(state),
        providerWasInitialized
    );
}

void updateFpsCounters(AppWindowState& state)
{
    updateFpsCounters(static_cast<AppRuntimeState&>(state));
}

} // namespace ovtr::win32
