#include "platform/win32/TopMenuSettingsActions.h"

#include "platform/win32/AppConfig.h"
#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/WindowLayout.h"

namespace ovtr::win32 {

bool showViewportColorSettings(HWND parent, AppWindowState& state)
{
    ViewportSettings result;
    if (!promptForViewportColorSettings(parent, state.viewportSettings, result)) {
        return false;
    }

    state.viewportSettings = result;
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
    input.initialRecordDelaySeconds = sanitizedRecordDelaySeconds(state.recordDelaySeconds);
    input.initialExportSampleRate = sanitizedRecordExportSampleRate(state.recordExportSampleRate);
    input.initialSaveFormat = state.recordSaveFormat;
    input.defaultExportSampleRate = kDefaultRecordExportSampleRate;

    RecordSettingsDialogResult result;
    if (!promptForRecordSettings(parent, input, result)) {
        return false;
    }

    state.exportDirectory = result.directory;
    state.recordDelaySeconds = result.recordDelaySeconds;
    state.recordExportSampleRate = result.exportSampleRate;
    state.recordSaveFormat = result.saveFormat;
    state.exportStatusMessage = "Record settings updated";
    appendDebugLog(state, state.exportStatusMessage);
    saveRecordSettingsConfig(state);
    invalidateStatusPanel(parent);
    InvalidateRect(parent, nullptr, FALSE);
    return true;
}

} // namespace ovtr::win32
