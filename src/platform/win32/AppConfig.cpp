#include "platform/win32/AppConfig.h"

#include "platform/win32/AppRecordingState.h"
#include "platform/win32/AppState.h"
#include "platform/win32/AppStreamingState.h"
#include "platform/win32/ConfigStore.h"

namespace ovtr::win32 {

std::filesystem::path activeExportDirectoryPath(const AppRecordingState& state)
{
    return normalizedExportDirectoryPath(state.exportDirectory);
}

float sanitizedRecordExportSampleRate(const float value) noexcept
{
    return sanitizedExportSampleRate(value, kDefaultRecordExportSampleRate);
}

void saveDeviceNameConfig(AppWindowState& state)
{
    saveDeviceNameConfig(
        static_cast<AppDeviceState&>(state),
        static_cast<AppDebugUiState&>(state)
    );
}

void loadDeviceNameConfig(AppWindowState& state)
{
    loadDeviceNameConfig(
        static_cast<AppDeviceState&>(state),
        static_cast<AppDebugUiState&>(state)
    );
}

void saveRecordSettingsConfig(AppWindowState& state)
{
    saveRecordSettingsConfig(
        static_cast<AppRecordingState&>(state),
        static_cast<AppDebugUiState&>(state)
    );
}

void loadRecordSettingsConfig(AppWindowState& state)
{
    loadRecordSettingsConfig(
        static_cast<AppRecordingState&>(state),
        static_cast<AppDebugUiState&>(state)
    );
}

void saveStreamingSettingsConfig(AppWindowState& state)
{
    saveStreamingSettingsConfig(
        static_cast<AppStreamingState&>(state),
        static_cast<AppDebugUiState&>(state)
    );
}

void loadStreamingSettingsConfig(AppWindowState& state)
{
    loadStreamingSettingsConfig(
        static_cast<AppStreamingState&>(state),
        static_cast<AppDebugUiState&>(state)
    );
}

void saveOriginConfig(AppWindowState& state)
{
    saveOriginConfig(
        static_cast<AppOriginState&>(state),
        static_cast<AppDebugUiState&>(state)
    );
}

void loadOriginConfig(AppWindowState& state)
{
    loadOriginConfig(
        static_cast<AppOriginState&>(state),
        static_cast<AppDebugUiState&>(state)
    );
}

void saveViewportSettingsConfig(AppWindowState& state)
{
    saveViewportSettingsConfig(
        static_cast<AppViewportState&>(state),
        static_cast<AppDebugUiState&>(state)
    );
}

void loadViewportSettingsConfig(AppWindowState& state)
{
    loadViewportSettingsConfig(
        static_cast<AppViewportState&>(state),
        static_cast<AppDebugUiState&>(state)
    );
    if (!state.mappingSkeletonColorCustomized) {
        state.mappingSkeletonColor = state.viewportSettings.bodyColor;
    }
}

} // namespace ovtr::win32
