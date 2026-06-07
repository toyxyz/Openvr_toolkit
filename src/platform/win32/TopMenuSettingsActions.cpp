#include "platform/win32/TopMenuSettingsActions.h"

#include "platform/win32/AppConfig.h"
#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/Win32String.h"
#include "platform/win32/WindowLayout.h"

#include <mutex>

namespace ovtr::win32 {

bool showViewportColorSettings(HWND parent, AppWindowState& state)
{
    ViewportSettings result;
    if (!promptForViewportColorSettings(parent, state.viewportSettings, result)) {
        return false;
    }

    state.viewportSettings = result;
    if (!state.mappingSkeletonColorCustomized) {
        state.mappingSkeletonColor = state.viewportSettings.bodyColor;
    }
    for (SceneMarker& marker : state.markers) {
        marker.sizeMeters = state.viewportSettings.markerSize;
    }
    saveViewportSettingsConfig(state);
    appendDebugLog(state, L"Viewport appearance settings applied");
    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    return true;
}

bool showExportLocationSettings(HWND parent, AppWindowState& state)
{
    RecordSettingsDialogInput input;
    input.initialDirectory = activeExportDirectoryPath(state);
    input.initialSessionDirectory = activeSessionDirectoryPath(state);
    input.initialRecordDelaySeconds = sanitizedRecordDelaySeconds(state.recordDelaySeconds);
    input.initialExportSampleRate = sanitizedRecordExportSampleRate(state.recordExportSampleRate);
    input.initialStartRecordingOnCalibration = state.startRecordingOnCalibration;
    input.initialExportAfterRecording = state.exportAfterRecording;
    input.initialApplyNoiseFilterOnExport = state.applyNoiseFilterOnExport;
    input.initialNoiseFilterCutoffHz = sanitizedNoiseFilterCutoffHz(state.noiseFilterCutoffHz);
    input.initialOutlierRepairStrength = state.outlierRepairStrength;
    input.initialSmoothingIterations = state.smoothingIterations;
    input.defaultExportSampleRate = kDefaultRecordExportSampleRate;

    RecordSettingsDialogResult result;
    if (!promptForRecordSettings(parent, input, result)) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(state.recordingMutex);
        state.exportDirectory = result.directory;
        state.sessionDirectory = result.sessionDirectory;
        state.recordDelaySeconds = result.recordDelaySeconds;
        state.recordExportSampleRate = result.exportSampleRate;
        state.startRecordingOnCalibration = result.startRecordingOnCalibration;
        state.exportAfterRecording = result.exportAfterRecording;
        state.applyNoiseFilterOnExport = result.applyNoiseFilterOnExport;
        state.noiseFilterCutoffHz = result.noiseFilterCutoffHz;
        state.outlierRepairStrength = result.outlierRepairStrength;
        state.smoothingIterations = result.smoothingIterations;
    }
    state.exportStatusMessage = "Record settings updated";
    appendDebugLog(state, state.exportStatusMessage);
    saveRecordSettingsConfig(state);
    invalidateStatusPanel(parent);
    InvalidateRect(parent, nullptr, FALSE);
    return true;
}

bool showStreamingSettings(HWND parent, AppWindowState& state)
{
    StreamingSettingsConfig input;
    {
        std::lock_guard<std::mutex> lock(state.realtimeSmoothingMutex);
        input.realtimeSmoothingEnabled = state.realtimeSmoothingEnabled;
        input.realtimeSmoothingPreset = state.realtimeSmoothingPreset;
        input.vmcReceiveEnabled = state.vmcReceiveEnabled;
        input.vmcPort = state.vmcPort;
    }

    StreamingSettingsConfig result;
    if (!promptForStreamingSettings(parent, input, result)) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(state.realtimeSmoothingMutex);
        state.realtimeSmoothingEnabled = result.realtimeSmoothingEnabled;
        state.realtimeSmoothingPreset = result.realtimeSmoothingPreset;
        state.realtimePoseSmoother.setPreset(result.realtimeSmoothingPreset);
        state.realtimePoseSmoother.reset();
        state.vmcReceiveEnabled = result.vmcReceiveEnabled;
        state.vmcPort = result.vmcPort;
        std::string vmcError;
        if (!state.vmcReceiver.configure(state.vmcReceiveEnabled, state.vmcPort, vmcError)) {
            state.vmcReceiveEnabled = false;
            MessageBoxW(parent, widen(vmcError).c_str(), L"VMC Receive", MB_OK | MB_ICONERROR);
            appendDebugLog(state, "VMC receive start failed: " + vmcError);
        }
    }
    appendDebugLog(
        state,
        std::string("Streaming settings updated: smoothing ") +
            (result.realtimeSmoothingEnabled ? "on, " : "off, ") +
            realtimeSmoothingPresetConfigValue(result.realtimeSmoothingPreset) +
            ", VMC receive " + (state.vmcReceiveEnabled ? "on" : "off")
    );
    saveStreamingSettingsConfig(state);
    invalidateStatusPanel(parent);
    InvalidateRect(parent, nullptr, FALSE);
    return true;
}

} // namespace ovtr::win32
