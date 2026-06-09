#include "platform/win32/AppConfig.h"

#include "platform/win32/AppDebugUiState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/AppRecordingState.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/Win32String.h"

#include <fstream>

namespace ovtr::win32 {

bool writeRecordSettingsConfigFile(const AppRecordingState& state, std::string& error)
{
    const std::filesystem::path path = recordSettingsConfigPath();
    if (!ensureConfigDirectory(error)) {
        return false;
    }

    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        error = "could not open " + path.string();
        return false;
    }

    output << serializeRecordSettingsConfig(
        narrow(activeExportDirectoryPath(state).wstring()),
        narrow(activeSessionDirectoryPath(state).wstring()),
        state.recordDelaySeconds,
        state.recordExportSampleRate,
        state.startRecordingOnCalibration,
        state.exportAfterRecording,
        state.applyNoiseFilterOnExport,
        state.noiseFilterCutoffHz,
        state.outlierRepairStrength,
        state.smoothingIterations,
        kDefaultRecordExportSampleRate
    );
    return true;
}

void saveRecordSettingsConfig(AppRecordingState& state, AppDebugUiState& logState)
{
    state.exportDirectory = activeExportDirectoryPath(state);
    state.sessionDirectory = activeSessionDirectoryPath(state);
    state.recordDelaySeconds = sanitizedRecordDelaySeconds(state.recordDelaySeconds);
    state.recordExportSampleRate = sanitizedRecordExportSampleRate(state.recordExportSampleRate);
    state.noiseFilterCutoffHz = sanitizedNoiseFilterCutoffHz(state.noiseFilterCutoffHz);
    state.smoothingIterations = sanitizedSmoothingIterations(state.smoothingIterations);
    std::string error;
    if (!writeRecordSettingsConfigFile(state, error)) {
        appendDebugLog(logState, "Record settings config save failed: " + error);
        return;
    }
    appendDebugLog(logState, "Record settings config saved: " + recordSettingsConfigPath().string());
}

void loadRecordSettingsConfig(AppRecordingState& state, AppDebugUiState& logState)
{
    std::filesystem::path path = readableConfigPath(kRecordSettingsConfigFileName);
    std::ifstream input(path);
    if (!input) {
        path = readableConfigPath(kLegacyExportLocationConfigFileName);
        input.clear();
        input.open(path);
    }
    if (!input) {
        state.exportDirectory = defaultExportDirectoryPath();
        state.sessionDirectory = defaultSessionDirectoryPath();
        state.recordDelaySeconds = 0.0f;
        state.recordExportSampleRate = kDefaultRecordExportSampleRate;
        state.startRecordingOnCalibration = false;
        state.exportAfterRecording = false;
        state.applyNoiseFilterOnExport = false;
        state.noiseFilterCutoffHz = 8.0f;
        state.outlierRepairStrength = OutlierRepairStrength::Light;
        state.smoothingIterations = kDefaultSmoothingIterations;
        appendDebugLog(logState, "Record settings config not found: " + path.string());
        return;
    }

    const RecordSettingsConfig config = parseRecordSettingsConfig(input, kDefaultRecordExportSampleRate);

    state.exportDirectory = normalizedExportDirectoryPath(std::filesystem::path(widen(config.exportDirectoryText)));
    state.sessionDirectory = normalizedSessionDirectoryPath(std::filesystem::path(widen(config.sessionDirectoryText)));
    state.recordDelaySeconds = config.recordDelaySeconds;
    state.recordExportSampleRate = config.exportSampleRate;
    state.startRecordingOnCalibration = config.startRecordingOnCalibration;
    state.exportAfterRecording = config.exportAfterRecording;
    state.applyNoiseFilterOnExport = config.applyNoiseFilterOnExport;
    state.noiseFilterCutoffHz = config.noiseFilterCutoffHz;
    state.outlierRepairStrength = config.outlierRepairStrength;
    state.smoothingIterations = config.smoothingIterations;
    appendDebugLog(logState, "Record settings config loaded: " + path.string());
    if (path.lexically_normal() != recordSettingsConfigPath().lexically_normal()) {
        saveRecordSettingsConfig(state, logState);
    }
}

} // namespace ovtr::win32
