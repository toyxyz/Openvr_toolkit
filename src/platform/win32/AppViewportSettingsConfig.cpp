#include "platform/win32/AppConfig.h"

#include "platform/win32/AppDebugUiState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/AppViewportState.h"
#include "platform/win32/ConfigStore.h"

#include <fstream>

namespace ovtr::win32 {

bool writeViewportSettingsConfigFile(const AppViewportState& state, std::string& error)
{
    const std::filesystem::path path = viewportSettingsConfigPath();
    if (!ensureConfigDirectory(error)) {
        return false;
    }

    const ViewportSettings& settings = state.viewportSettings;
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        error = "could not open " + path.string();
        return false;
    }

    output << serializeViewportSettingsConfig(settings);
    return true;
}

void saveViewportSettingsConfig(AppViewportState& state, AppDebugUiState& logState)
{
    state.viewportSettings = clampViewportSettings(state.viewportSettings);

    std::string error;
    if (!writeViewportSettingsConfigFile(state, error)) {
        appendDebugLog(logState, "Viewport settings save failed: " + error);
        return;
    }
    appendDebugLog(logState, "Viewport settings saved: " + viewportSettingsConfigPath().string());
}

void loadViewportSettingsConfig(AppViewportState& state, AppDebugUiState& logState)
{
    const std::filesystem::path path = readableConfigPath(kViewportSettingsConfigFileName);
    std::ifstream input(path);
    if (!input) {
        appendDebugLog(logState, "Viewport settings config not found: " + path.string());
        return;
    }

    state.viewportSettings = parseViewportSettingsConfig(input, state.viewportSettings);
    appendDebugLog(logState, "Viewport settings loaded: " + path.string());
    if (path.lexically_normal() != viewportSettingsConfigPath().lexically_normal()) {
        saveViewportSettingsConfig(state, logState);
    }
}

} // namespace ovtr::win32
