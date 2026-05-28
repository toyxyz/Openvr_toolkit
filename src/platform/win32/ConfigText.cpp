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

const char* exportFormatConfigValue(const ExportFormat format) noexcept
{
    return format == ExportFormat::Fbx ? "fbx" : "glb";
}

int clampColorComponent(const int value) noexcept
{
    return std::clamp(value, 0, 255);
}

} // namespace ovtr::win32
