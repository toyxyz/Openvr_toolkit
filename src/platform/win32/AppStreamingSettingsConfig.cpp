#include "platform/win32/AppConfig.h"

#include "platform/win32/AppDebugUiState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/AppStreamingState.h"
#include "platform/win32/ConfigStore.h"

#include <fstream>
#include <mutex>

namespace ovtr::win32 {

bool writeStreamingSettingsConfigFile(const AppStreamingState& state, std::string& error)
{
    const std::filesystem::path path = streamingSettingsConfigPath();
    if (!ensureConfigDirectory(error)) {
        return false;
    }

    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        error = "could not open " + path.string();
        return false;
    }

    StreamingSettingsConfig config;
    {
        std::lock_guard<std::mutex> lock(state.realtimeSmoothingMutex);
        config.realtimeSmoothingEnabled = state.realtimeSmoothingEnabled;
        config.realtimeSmoothingPreset = state.realtimeSmoothingPreset;
        config.vmcReceiveEnabled = state.vmcReceiveEnabled;
        config.vmcPort = state.vmcPort;
    }
    output << serializeStreamingSettingsConfig(config);
    return true;
}

void saveStreamingSettingsConfig(AppStreamingState& state, AppDebugUiState& logState)
{
    std::string error;
    if (!writeStreamingSettingsConfigFile(state, error)) {
        appendDebugLog(logState, "Streaming settings config save failed: " + error);
        return;
    }
    appendDebugLog(logState, "Streaming settings config saved: " + streamingSettingsConfigPath().string());
}

void loadStreamingSettingsConfig(AppStreamingState& state, AppDebugUiState& logState)
{
    const std::filesystem::path path = readableConfigPath(kStreamingSettingsConfigFileName);
    std::ifstream input(path);
    if (!input) {
        appendDebugLog(logState, "Streaming settings config not found: " + path.string());
        return;
    }

    const StreamingSettingsConfig config = parseStreamingSettingsConfig(input);
    {
        std::lock_guard<std::mutex> lock(state.realtimeSmoothingMutex);
        state.realtimeSmoothingEnabled = config.realtimeSmoothingEnabled;
        state.realtimeSmoothingPreset = config.realtimeSmoothingPreset;
        state.realtimePoseSmoother.setPreset(config.realtimeSmoothingPreset);
        state.realtimePoseSmoother.reset();
        state.vmcReceiveEnabled = config.vmcReceiveEnabled;
        state.vmcPort = config.vmcPort;
    }
    if (state.vmcReceiveEnabled) {
        std::string error;
        if (!state.vmcReceiver.configure(true, state.vmcPort, error)) {
            state.vmcReceiveEnabled = false;
            appendDebugLog(logState, "VMC receive start failed: " + error);
        }
    }
    appendDebugLog(logState, "Streaming settings config loaded: " + path.string());
}

} // namespace ovtr::win32
