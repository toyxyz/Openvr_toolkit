#include "platform/win32/ConfigTextInternal.h"

#include <iomanip>
#include <sstream>

namespace ovtr::win32 {

RecordSettingsConfig parseRecordSettingsConfig(std::istream& input, const float defaultSampleRate)
{
    RecordSettingsConfig config;
    config.exportSampleRate = defaultSampleRate;

    std::string line;
    while (std::getline(input, line)) {
        detail::ConfigAssignment assignment;
        if (!detail::parseConfigAssignmentLine(line, assignment)) {
            const std::string value = trimAscii(line);
            if (!value.empty() && value.front() != '#') {
                config.exportDirectoryText = value;
            }
            continue;
        }

        const std::string& key = assignment.key;
        const std::string& value = assignment.value;
        if (key == "directory" || key == "path" || key == "export_directory") {
            config.exportDirectoryText = value;
        } else if (key == "record_delay_seconds" || key == "record_delay" || key == "delay_seconds") {
            float parsedDelay = 0.0f;
            if (parseFloatConfigValue(value, parsedDelay)) {
                config.recordDelaySeconds = parsedDelay;
            }
        } else if (key == "resample_fps" || key == "export_sample_rate" || key == "export_fps") {
            float parsedSampleRate = 0.0f;
            if (parseFloatConfigValue(value, parsedSampleRate)) {
                config.exportSampleRate = parsedSampleRate;
            }
        } else if (key == "save_format" || key == "format" || key == "export_format") {
            parseExportFormatConfigValue(value, config.saveFormat);
        }
    }

    config.recordDelaySeconds = sanitizedRecordDelaySeconds(config.recordDelaySeconds);
    config.exportSampleRate = sanitizedExportSampleRate(config.exportSampleRate, defaultSampleRate);
    return config;
}

std::string serializeRecordSettingsConfig(
    const std::string& exportDirectoryText,
    const float recordDelaySeconds,
    const float exportSampleRate,
    const ExportFormat saveFormat,
    const float defaultSampleRate
)
{
    std::ostringstream output;
    output << "directory=" << exportDirectoryText << "\n";
    output << "record_delay_seconds=" << std::fixed << std::setprecision(3)
           << sanitizedRecordDelaySeconds(recordDelaySeconds) << "\n";
    output << "resample_fps=" << sanitizedExportSampleRate(exportSampleRate, defaultSampleRate) << "\n";
    output << "save_format=" << exportFormatConfigValue(saveFormat) << "\n";
    return output.str();
}

} // namespace ovtr::win32
