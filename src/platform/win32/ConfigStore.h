#pragma once

#include "platform/win32/ConfigTypes.h"

#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace ovtr::win32 {

inline constexpr const char* kOriginConfigFileName = "openvr_tracker_recorder_origin.cfg";
inline constexpr const char* kDeviceNameConfigFileName = "openvr_tracker_recorder_device_names.cfg";
inline constexpr const char* kViewportSettingsConfigFileName = "openvr_tracker_recorder_viewport.cfg";
inline constexpr const char* kRecordSettingsConfigFileName = "openvr_tracker_recorder_record.cfg";
inline constexpr const char* kStreamingSettingsConfigFileName = "openvr_tracker_recorder_streaming.cfg";
inline constexpr const char* kLegacyExportLocationConfigFileName = "openvr_tracker_recorder_export_location.cfg";
inline constexpr const char* kRenderModelMatcapTextureFileName = "render_model_matcap.png";

std::string trimAscii(std::string value);
std::string lowerAscii(std::string value);

std::filesystem::path executableDirectoryPath();
std::filesystem::path configDirectoryPath();
std::filesystem::path configFilePath(const char* fileName);
std::filesystem::path legacyExecutableConfigPath(const char* fileName);
std::filesystem::path legacyWorkingDirectoryConfigPath(const char* fileName);
std::filesystem::path readableConfigPath(const char* fileName);
bool ensureConfigDirectory(std::string& error);
std::filesystem::path originConfigPath();
std::filesystem::path deviceNameConfigPath();
std::filesystem::path viewportSettingsConfigPath();
std::filesystem::path recordSettingsConfigPath();
std::filesystem::path streamingSettingsConfigPath();
std::filesystem::path defaultExportDirectoryPath();
std::filesystem::path normalizedExportDirectoryPath(const std::filesystem::path& path);

bool parseExportFormatConfigValue(const std::string& value, ExportFormat& out);
bool parseBoolConfigValue(const std::string& value, bool& out);
bool parseFloatConfigValue(const std::string& value, float& out);
bool parseIntConfigValue(const std::string& value, int& out);
bool parseOutlierRepairStrengthConfigValue(const std::string& value, OutlierRepairStrength& out);
bool parseSmoothingStrengthConfigValue(const std::string& value, SmoothingStrength& out);
bool parseRealtimeSmoothingPresetConfigValue(const std::string& value, RealtimeSmoothingPreset& out);

float sanitizedRecordDelaySeconds(float value) noexcept;
float sanitizedExportSampleRate(float value, float fallback) noexcept;
float sanitizedNoiseFilterCutoffHz(float value) noexcept;
int sanitizedSmoothingIterations(int value) noexcept;
const char* exportFormatConfigValue(ExportFormat format) noexcept;
const char* outlierRepairStrengthConfigValue(OutlierRepairStrength strength) noexcept;
const char* realtimeSmoothingPresetConfigValue(RealtimeSmoothingPreset preset) noexcept;
int smoothingIterationsForStrength(SmoothingStrength strength) noexcept;

int clampColorComponent(int value) noexcept;
RgbColor clampRgbColor(RgbColor color) noexcept;
ViewportSettings clampViewportSettings(ViewportSettings settings) noexcept;

DeviceNameConfigParseResult parseDeviceNameConfig(std::istream& input);
std::string serializeDeviceNameConfig(const std::vector<DeviceNameConfigEntry>& entries);

RecordSettingsConfig parseRecordSettingsConfig(std::istream& input, float defaultSampleRate);
std::string serializeRecordSettingsConfig(
    const std::string& exportDirectoryText,
    float recordDelaySeconds,
    float exportSampleRate,
    bool startRecordingOnCalibration,
    bool exportAfterRecording,
    bool applyNoiseFilterOnExport,
    float noiseFilterCutoffHz,
    OutlierRepairStrength outlierRepairStrength,
    int smoothingIterations,
    float defaultSampleRate
);

StreamingSettingsConfig parseStreamingSettingsConfig(std::istream& input);
std::string serializeStreamingSettingsConfig(const StreamingSettingsConfig& config);

OriginConfigParseResult parseOriginConfig(std::istream& input);
std::string serializeOriginConfig(const OriginConfig& config);

ViewportSettings parseViewportSettingsConfig(std::istream& input, ViewportSettings defaults);
std::string serializeViewportSettingsConfig(ViewportSettings settings);

} // namespace ovtr::win32
