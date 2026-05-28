#include "platform/win32/ConfigTextInternal.h"

#include <iomanip>
#include <sstream>

namespace ovtr::win32 {

OriginConfigParseResult parseOriginConfig(std::istream& input)
{
    bool hasEnabled = false;
    bool hasX = false;
    bool hasY = false;
    bool hasZ = false;
    OriginConfig config;

    std::string line;
    while (std::getline(input, line)) {
        detail::ConfigAssignment assignment;
        if (!detail::parseConfigAssignmentLine(line, assignment)) {
            continue;
        }

        const std::string& key = assignment.key;
        const std::string& value = assignment.value;
        if (key == "enabled") {
            hasEnabled = parseBoolConfigValue(value, config.enabled);
        } else if (key == "x") {
            hasX = parseFloatConfigValue(value, config.offset[0]);
        } else if (key == "y") {
            hasY = parseFloatConfigValue(value, config.offset[1]);
        } else if (key == "z") {
            hasZ = parseFloatConfigValue(value, config.offset[2]);
        } else if (key == "rx") {
            parseFloatConfigValue(value, config.rotationDegrees[0]);
        } else if (key == "ry") {
            parseFloatConfigValue(value, config.rotationDegrees[1]);
        } else if (key == "rz") {
            parseFloatConfigValue(value, config.rotationDegrees[2]);
        }
    }

    if (!hasEnabled) {
        return {OriginConfigParseStatus::MissingEnabled, config};
    }
    if (!config.enabled) {
        return {OriginConfigParseStatus::Disabled, config};
    }
    if (!hasX || !hasY || !hasZ) {
        return {OriginConfigParseStatus::MissingCoordinates, config};
    }
    return {OriginConfigParseStatus::Loaded, config};
}

std::string serializeOriginConfig(const OriginConfig& config)
{
    std::ostringstream output;
    output << "enabled=" << (config.enabled ? 1 : 0) << "\n";
    output << std::fixed << std::setprecision(9)
           << "x=" << config.offset[0] << "\n"
           << "y=" << config.offset[1] << "\n"
           << "z=" << config.offset[2] << "\n"
           << "rx=" << config.rotationDegrees[0] << "\n"
           << "ry=" << config.rotationDegrees[1] << "\n"
           << "rz=" << config.rotationDegrees[2] << "\n";
    return output.str();
}

} // namespace ovtr::win32
