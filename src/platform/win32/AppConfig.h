#pragma once

#include <filesystem>
#include <string>

namespace ovtr::win32 {

struct AppDeviceState;
struct AppDebugUiState;
struct AppOriginState;
struct AppRecordingState;
struct AppViewportState;
struct AppWindowState;

constexpr float kDefaultRecordExportSampleRate = 60.0f;

std::filesystem::path activeExportDirectoryPath(const AppRecordingState& state);
float sanitizedRecordExportSampleRate(float value) noexcept;

bool writeDeviceNameConfigFile(const AppDeviceState& state, std::string& error);
void saveDeviceNameConfig(AppDeviceState& state, AppDebugUiState& logState);
void loadDeviceNameConfig(AppDeviceState& state, AppDebugUiState& logState);
void saveDeviceNameConfig(AppWindowState& state);
void loadDeviceNameConfig(AppWindowState& state);

bool writeRecordSettingsConfigFile(const AppRecordingState& state, std::string& error);
void saveRecordSettingsConfig(AppRecordingState& state, AppDebugUiState& logState);
void loadRecordSettingsConfig(AppRecordingState& state, AppDebugUiState& logState);
void saveRecordSettingsConfig(AppWindowState& state);
void loadRecordSettingsConfig(AppWindowState& state);

bool writeOriginConfigFile(const AppOriginState& state, std::string& error);
void saveOriginConfig(AppOriginState& state, AppDebugUiState& logState);
void loadOriginConfig(AppOriginState& state, AppDebugUiState& logState);
void saveOriginConfig(AppWindowState& state);
void loadOriginConfig(AppWindowState& state);

bool writeViewportSettingsConfigFile(const AppViewportState& state, std::string& error);
void saveViewportSettingsConfig(AppViewportState& state, AppDebugUiState& logState);
void loadViewportSettingsConfig(AppViewportState& state, AppDebugUiState& logState);
void saveViewportSettingsConfig(AppWindowState& state);
void loadViewportSettingsConfig(AppWindowState& state);

} // namespace ovtr::win32
