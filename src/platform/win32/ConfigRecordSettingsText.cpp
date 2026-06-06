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
        } else if (key == "start_recording_on_calibration") {
            parseBoolConfigValue(value, config.startRecordingOnCalibration);
        } else if (key == "export_after_recording") {
            parseBoolConfigValue(value, config.exportAfterRecording);
        } else if (key == "apply_noise_filter_on_export") {
            parseBoolConfigValue(value, config.applyNoiseFilterOnExport);
        } else if (key == "noise_filter_cutoff_hz") {
            float parsedCutoff = 0.0f;
            if (parseFloatConfigValue(value, parsedCutoff)) {
                config.noiseFilterCutoffHz = parsedCutoff;
            }
        } else if (key == "outlier_repair_strength") {
            parseOutlierRepairStrengthConfigValue(value, config.outlierRepairStrength);
        } else if (key == "smoothing_iterations") {
            int parsedIterations = 0;
            if (parseIntConfigValue(value, parsedIterations)) {
                config.smoothingIterations = parsedIterations;
            }
        } else if (key == "smoothing_strength" || key == "gaussian_smoothing_strength") {
            SmoothingStrength parsedStrength = SmoothingStrength::None;
            if (parseSmoothingStrengthConfigValue(value, parsedStrength)) {
                config.smoothingIterations = smoothingIterationsForStrength(parsedStrength);
            }
        }
    }

    config.recordDelaySeconds = sanitizedRecordDelaySeconds(config.recordDelaySeconds);
    config.exportSampleRate = sanitizedExportSampleRate(config.exportSampleRate, defaultSampleRate);
    config.noiseFilterCutoffHz = sanitizedNoiseFilterCutoffHz(config.noiseFilterCutoffHz);
    config.smoothingIterations = sanitizedSmoothingIterations(config.smoothingIterations);
    return config;
}

std::string serializeRecordSettingsConfig(
    const std::string& exportDirectoryText,
    const float recordDelaySeconds,
    const float exportSampleRate,
    const bool startRecordingOnCalibration,
    const bool exportAfterRecording,
    const bool applyNoiseFilterOnExport,
    const float noiseFilterCutoffHz,
    const OutlierRepairStrength outlierRepairStrength,
    const int smoothingIterations,
    const float defaultSampleRate
)
{
    std::ostringstream output;
    output << "directory=" << exportDirectoryText << "\n";
    output << "record_delay_seconds=" << std::fixed << std::setprecision(3)
           << sanitizedRecordDelaySeconds(recordDelaySeconds) << "\n";
    output << "resample_fps=" << sanitizedExportSampleRate(exportSampleRate, defaultSampleRate) << "\n";
    output << "start_recording_on_calibration="
           << (startRecordingOnCalibration ? "true" : "false") << "\n";
    output << "export_after_recording="
           << (exportAfterRecording ? "true" : "false") << "\n";
    output << "apply_noise_filter_on_export="
           << (applyNoiseFilterOnExport ? "true" : "false") << "\n";
    output << "noise_filter_cutoff_hz=" << std::fixed << std::setprecision(3)
           << sanitizedNoiseFilterCutoffHz(noiseFilterCutoffHz) << "\n";
    output << "outlier_repair_strength="
           << outlierRepairStrengthConfigValue(outlierRepairStrength) << "\n";
    output << "smoothing_iterations=" << sanitizedSmoothingIterations(smoothingIterations) << "\n";
    return output.str();
}

} // namespace ovtr::win32
