#pragma once

#include <array>
#include <string>
#include <vector>

namespace ovtr::win32 {

enum class ExportFormat {
    Fbx,
    Glb,
};

struct RgbColor {
    int r = 0;
    int g = 0;
    int b = 0;
};

enum class SkeletonDisplayType {
    Line,
    Box,
};

struct ViewportSettings {
    RgbColor labelTextColor{255, 255, 255};
    RgbColor gridColor{34, 36, 43};
    RgbColor backgroundColor{37, 50, 65};
    RgbColor importedGlbColor{66, 104, 255};
    RgbColor renderModelOutlineColor{255, 133, 32};
    RgbColor renderModelMaterialColor{255, 255, 255};
    RgbColor fingerBoxColor{255, 230, 128};
    RgbColor markerColor{255, 255, 255};
    RgbColor bodyColor{255, 255, 255};
    SkeletonDisplayType skeletonDisplayType = SkeletonDisplayType::Box;
    float outlineMultiplier = 1.0f;
    float gridSize = 5.0f;
    float gridCellDensity = 2.0f;
    float markerSize = 0.10f;
};

struct DeviceNameConfigEntry {
    std::string deviceClass;
    std::string serial;
    std::string customName;
};

struct DeviceNameConfigParseResult {
    std::vector<DeviceNameConfigEntry> entries;
    int invalidLineCount = 0;
};

enum class OutlierRepairStrength {
    None,
    Light,
    Normal,
    Strong,
};

enum class SmoothingStrength {
    None,
    Light,
    Normal,
    Strong,
};

enum class RealtimeSmoothingPreset {
    VeryLight,
    Light,
    Normal,
    Strong,
    VeryStrong,
};

struct RecordSettingsConfig {
    std::string exportDirectoryText;
    std::string sessionDirectoryText;
    float recordDelaySeconds = 0.0f;
    float exportSampleRate = 60.0f;
    bool startRecordingOnCalibration = false;
    bool exportAfterRecording = false;
    bool applyNoiseFilterOnExport = false;
    float noiseFilterCutoffHz = 8.0f;
    OutlierRepairStrength outlierRepairStrength = OutlierRepairStrength::Light;
    int smoothingIterations = 0;
};

struct StreamingSettingsConfig {
    bool realtimeSmoothingEnabled = false;
    RealtimeSmoothingPreset realtimeSmoothingPreset = RealtimeSmoothingPreset::Normal;
    bool vmcReceiveEnabled = false;
    int vmcPort = 39540;
};

struct OriginConfig {
    bool enabled = false;
    std::array<float, 3> offset{0.0f, 0.0f, 0.0f};
    std::array<float, 3> rotationDegrees{0.0f, 0.0f, 0.0f};
};

enum class OriginConfigParseStatus {
    Loaded,
    Disabled,
    MissingEnabled,
    MissingCoordinates,
};

struct OriginConfigParseResult {
    OriginConfigParseStatus status = OriginConfigParseStatus::MissingEnabled;
    OriginConfig config;
};

} // namespace ovtr::win32
