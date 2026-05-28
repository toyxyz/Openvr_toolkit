#pragma once

namespace ovtr::win32 {

struct AppDebugUiState;
struct AppRuntimeState;
struct AppWindowState;

void refreshProviderAndDevices(AppWindowState& state, bool forceDeviceEnumeration);
void appendProviderStatusLogChanges(
    AppRuntimeState& runtimeState,
    AppDebugUiState& logState,
    bool providerWasInitialized
);
void appendProviderStatusLogChanges(AppWindowState& state, bool providerWasInitialized);
void updateFpsCounters(AppRuntimeState& state);
void updateFpsCounters(AppWindowState& state);

} // namespace ovtr::win32
