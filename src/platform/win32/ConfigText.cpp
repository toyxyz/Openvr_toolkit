#include "platform/win32/ConfigStore.h"

#include "export/ExportTypes.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <sstream>

namespace ovtr::win32 {

std::string trimAscii(std::string value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }
    return value;
}

std::string lowerAscii(std::string value)
{
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

bool parseExportFormatConfigValue(const std::string& value, ExportFormat& out)
{
    const std::string lowered = lowerAscii(trimAscii(value));
    if (lowered == "fbx") {
        out = ExportFormat::Fbx;
        return true;
    }
    if (lowered == "glb" || lowered == "gltf") {
        out = ExportFormat::Glb;
        return true;
    }
    return false;
}

bool parseBoolConfigValue(const std::string& value, bool& out)
{
    const std::string trimmed = trimAscii(value);
    if (trimmed == "1" || trimmed == "true" || trimmed == "True" || trimmed == "yes" || trimmed == "Yes") {
        out = true;
        return true;
    }
    if (trimmed == "0" || trimmed == "false" || trimmed == "False" || trimmed == "no" || trimmed == "No") {
        out = false;
        return true;
    }
    return false;
}

bool parseOutlierRepairStrengthConfigValue(const std::string& value, OutlierRepairStrength& out)
{
    const std::string lowered = lowerAscii(trimAscii(value));
    if (lowered == "none") {
        out = OutlierRepairStrength::None;
        return true;
    }
    if (lowered == "light") {
        out = OutlierRepairStrength::Light;
        return true;
    }
    if (lowered == "normal") {
        out = OutlierRepairStrength::Normal;
        return true;
    }
    if (lowered == "strong") {
        out = OutlierRepairStrength::Strong;
        return true;
    }
    return false;
}

bool parseSmoothingStrengthConfigValue(const std::string& value, SmoothingStrength& out)
{
    const std::string lowered = lowerAscii(trimAscii(value));
    if (lowered == "none") {
        out = SmoothingStrength::None;
        return true;
    }
    if (lowered == "light") {
        out = SmoothingStrength::Light;
        return true;
    }
    if (lowered == "normal") {
        out = SmoothingStrength::Normal;
        return true;
    }
    if (lowered == "strong") {
        out = SmoothingStrength::Strong;
        return true;
    }
    return false;
}

bool parseFloatConfigValue(const std::string& value, float& out)
{
    std::istringstream stream(trimAscii(value));
    stream >> out;
    return !stream.fail();
}

bool parseIntConfigValue(const std::string& value, int& out)
{
    std::istringstream stream(trimAscii(value));
    stream >> out;
    return !stream.fail();
}

float sanitizedRecordDelaySeconds(const float value) noexcept
{
    return std::isfinite(value) && value > 0.0f ? value : 0.0f;
}

float sanitizedExportSampleRate(const float value, const float fallback) noexcept
{
    return static_cast<float>(ovtr::sanitizedExportSampleRateValue(value, fallback));
}

float sanitizedNoiseFilterCutoffHz(const float value) noexcept
{
    constexpr float options[] = {0.5f, 1.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 15.0f, 20.0f};
    if (!std::isfinite(value)) {
        return 8.0f;
    }
    float best = options[0];
    float bestDelta = std::fabs(value - best);
    for (const float option : options) {
        const float delta = std::fabs(value - option);
        if (delta < bestDelta) {
            best = option;
            bestDelta = delta;
        }
    }
    return best;
}

int sanitizedSmoothingIterations(const int value) noexcept
{
    return std::clamp(value, 0, 100);
}

const char* exportFormatConfigValue(const ExportFormat format) noexcept
{
    return format == ExportFormat::Fbx ? "fbx" : "glb";
}

const char* outlierRepairStrengthConfigValue(const OutlierRepairStrength strength) noexcept
{
    switch (strength) {
    case OutlierRepairStrength::None:
        return "none";
    case OutlierRepairStrength::Normal:
        return "normal";
    case OutlierRepairStrength::Strong:
        return "strong";
    case OutlierRepairStrength::Light:
    default:
        return "light";
    }
}

int smoothingIterationsForStrength(const SmoothingStrength strength) noexcept
{
    switch (strength) {
    case SmoothingStrength::Light:
        return 1;
    case SmoothingStrength::Normal:
        return 2;
    case SmoothingStrength::Strong:
        return 4;
    case SmoothingStrength::None:
    default:
        return 0;
    }
}

int clampColorComponent(const int value) noexcept
{
    return std::clamp(value, 0, 255);
}

} // namespace ovtr::win32
