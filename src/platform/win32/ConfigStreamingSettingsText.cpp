#include "platform/win32/ConfigTextInternal.h"

#include <sstream>

namespace ovtr::win32 {

StreamingSettingsConfig parseStreamingSettingsConfig(std::istream& input)
{
    StreamingSettingsConfig config;

    std::string line;
    while (std::getline(input, line)) {
        detail::ConfigAssignment assignment;
        if (!detail::parseConfigAssignmentLine(line, assignment)) {
            continue;
        }

        if (assignment.key == "realtime_smoothing_enabled" ||
            assignment.key == "enabled") {
            parseBoolConfigValue(assignment.value, config.realtimeSmoothingEnabled);
        } else if (assignment.key == "realtime_smoothing_preset" ||
                   assignment.key == "preset") {
            parseRealtimeSmoothingPresetConfigValue(
                assignment.value,
                config.realtimeSmoothingPreset
            );
        }
    }

    return config;
}

std::string serializeStreamingSettingsConfig(const StreamingSettingsConfig& config)
{
    std::ostringstream output;
    output << "realtime_smoothing_enabled="
           << (config.realtimeSmoothingEnabled ? "true" : "false") << "\n";
    output << "realtime_smoothing_preset="
           << realtimeSmoothingPresetConfigValue(config.realtimeSmoothingPreset) << "\n";
    return output.str();
}

} // namespace ovtr::win32
