#include "platform/win32/ConfigTextInternal.h"

#include <sstream>

namespace ovtr::win32 {
namespace {

int sanitizedVmcPort(const int value) noexcept
{
    return value >= 1 && value <= 65535 ? value : 39540;
}

} // namespace

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
        } else if (assignment.key == "vmc_receive_enabled") {
            parseBoolConfigValue(assignment.value, config.vmcReceiveEnabled);
        } else if (assignment.key == "vmc_port") {
            int parsed = config.vmcPort;
            if (parseIntConfigValue(assignment.value, parsed)) {
                config.vmcPort = sanitizedVmcPort(parsed);
            }
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
    output << "vmc_receive_enabled="
           << (config.vmcReceiveEnabled ? "true" : "false") << "\n";
    output << "vmc_port=" << sanitizedVmcPort(config.vmcPort) << "\n";
    return output.str();
}

} // namespace ovtr::win32
