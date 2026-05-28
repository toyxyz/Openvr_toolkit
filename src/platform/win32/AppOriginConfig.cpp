#include "platform/win32/AppConfig.h"

#include "platform/win32/AppDebugUiState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/AppOriginState.h"
#include "platform/win32/ConfigStore.h"

#include <fstream>
#include <mutex>

namespace ovtr::win32 {

bool writeOriginConfigFile(const AppOriginState& state, std::string& error)
{
    const std::filesystem::path path = originConfigPath();
    if (!ensureConfigDirectory(error)) {
        return false;
    }

    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        error = "could not open " + path.string();
        return false;
    }

    OriginConfig config;
    config.enabled = state.originEnabled;
    config.offset = state.originOffset;
    config.rotationDegrees = state.originRotationDegrees;
    output << serializeOriginConfig(config);
    return true;
}

void saveOriginConfig(AppOriginState& state, AppDebugUiState& logState)
{
    std::string error;
    if (!writeOriginConfigFile(state, error)) {
        state.originStatusMessage = "origin config save failed: " + error;
        appendDebugLog(logState, state.originStatusMessage);
        return;
    }
    appendDebugLog(logState, "Origin config saved: " + originConfigPath().string());
}

void loadOriginConfig(AppOriginState& state, AppDebugUiState& logState)
{
    const std::filesystem::path path = readableConfigPath(kOriginConfigFileName);
    std::ifstream input(path);
    if (!input) {
        appendDebugLog(logState, "Origin config not found: " + path.string());
        return;
    }

    const OriginConfigParseResult config = parseOriginConfig(input);
    if (config.status == OriginConfigParseStatus::MissingEnabled) {
        appendDebugLog(logState, "Origin config ignored: missing enabled flag");
        return;
    }
    if (config.status == OriginConfigParseStatus::Disabled) {
        std::lock_guard<std::mutex> lock(state.originMutex);
        state.originEnabled = false;
        state.originOffset = config.config.offset;
        state.originRotationDegrees = config.config.rotationDegrees;
        state.originStatusMessage = "origin disabled by config";
        appendDebugLog(logState, "Origin config loaded: disabled");
        return;
    }
    if (config.status == OriginConfigParseStatus::MissingCoordinates) {
        appendDebugLog(logState, "Origin config ignored: missing origin coordinates");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(state.originMutex);
        state.originEnabled = true;
        state.originOffset = config.config.offset;
        state.originRotationDegrees = config.config.rotationDegrees;
        state.originStatusMessage = "origin loaded from config";
    }
    appendDebugLog(logState, "Origin config loaded: " + path.string());
    if (path.lexically_normal() != originConfigPath().lexically_normal()) {
        saveOriginConfig(state, logState);
    }
}

} // namespace ovtr::win32
