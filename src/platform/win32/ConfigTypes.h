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

struct ViewportSettings {
    RgbColor labelTextColor{255, 255, 255};
    RgbColor gridColor{34, 36, 43};
    RgbColor backgroundColor{37, 50, 65};
    RgbColor importedGlbColor{66, 104, 255};
    RgbColor renderModelOutlineColor{255, 133, 32};
    RgbColor renderModelMaterialColor{255, 255, 255};
    RgbColor fingerBoxColor{255, 230, 128};
    RgbColor markerColor{255, 255, 255};
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

struct RecordSettingsConfig {
    std::string exportDirectoryText;
    float recordDelaySeconds = 0.0f;
    float exportSampleRate = 60.0f;
    ExportFormat saveFormat = ExportFormat::Glb;
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
