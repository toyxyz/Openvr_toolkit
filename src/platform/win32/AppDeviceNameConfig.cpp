#include "platform/win32/AppConfig.h"

#include "platform/win32/AppDebugUiState.h"
#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/DeviceList.h"

#include <fstream>
#include <vector>

namespace ovtr::win32 {

bool writeDeviceNameConfigFile(const AppDeviceState& state, std::string& error)
{
    const std::filesystem::path path = deviceNameConfigPath();
    if (!ensureConfigDirectory(error)) {
        return false;
    }

    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        error = "could not open " + path.string();
        return false;
    }

    std::vector<DeviceNameConfigEntry> entries;
    entries.reserve(state.deviceCustomNames.size());
    for (const auto& entry : state.deviceCustomNames) {
        if (entry.second.empty()) {
            continue;
        }

        std::string deviceClass;
        std::string serial;
        if (!splitDeviceNameKey(entry.first, deviceClass, serial)) {
            continue;
        }

        entries.push_back({deviceClass, serial, entry.second});
    }
    output << serializeDeviceNameConfig(entries);
    return true;
}

void saveDeviceNameConfig(AppDeviceState& state, AppDebugUiState& logState)
{
    std::string error;
    if (!writeDeviceNameConfigFile(state, error)) {
        appendDebugLog(logState, "Device name config save failed: " + error);
        return;
    }
    appendDebugLog(logState, "Device name config saved: " + deviceNameConfigPath().string());
}

void loadDeviceNameConfig(AppDeviceState& state, AppDebugUiState& logState)
{
    const std::filesystem::path path = readableConfigPath(kDeviceNameConfigFileName);
    std::ifstream input(path);
    if (!input) {
        appendDebugLog(logState, "Device name config not found: " + path.string());
        return;
    }

    const DeviceNameConfigParseResult config = parseDeviceNameConfig(input);
    for (int ignored = 0; ignored < config.invalidLineCount; ++ignored) {
        appendDebugLog(logState, "Device name config ignored invalid line");
    }

    state.deviceCustomNames.clear();
    for (const DeviceNameConfigEntry& entry : config.entries) {
        state.deviceCustomNames[deviceNameKeyForParts(entry.deviceClass, entry.serial)] = entry.customName;
    }

    appendDebugLog(logState, "Device name config loaded: " + path.string());
    if (path.lexically_normal() != deviceNameConfigPath().lexically_normal()) {
        saveDeviceNameConfig(state, logState);
    }
}

} // namespace ovtr::win32
