#include "TestCases.h"
#include "TestSupport.h"

#include "platform/win32/AppState.h"
#include "platform/win32/DebugMonitorStatusLines.h"
#include "platform/win32/StatusPanel.h"

#include <chrono>

namespace ovtr::test {

void testWin32StatusPanel()
{
    ovtr::win32::AppWindowState state;
    const ovtr::win32::AppRecordingState& recordingState = state;
    const ovtr::win32::AppRuntimeState& runtimeState = state;
    const ovtr::win32::AppOriginState& originState = state;
    const ovtr::win32::AppImportedSceneState& importedSceneState = state;
    const ovtr::win32::AppViewportState& viewportState = state;
    const auto now = std::chrono::steady_clock::time_point{};

    require(
        ovtr::win32::exportFormatDisplayText(ovtr::win32::ExportFormat::Fbx) == L"fbx",
        "fbx display text"
    );
    require(
        ovtr::win32::exportFormatDisplayText(ovtr::win32::ExportFormat::Glb) == L"glb",
        "glb display text"
    );
    require(
        ovtr::win32::remainingRecordDelaySeconds(recordingState, now) == 0,
        "inactive recording delay has no remaining seconds"
    );

    state.recordingDelayActive = true;
    state.recordingDelayDeadline = now + std::chrono::milliseconds(1501);
    require(
        ovtr::win32::remainingRecordDelaySeconds(recordingState, now) == 2,
        "recording delay rounds remaining seconds up"
    );
    require(
        ovtr::win32::makeStatusBarMessage(runtimeState, recordingState, importedSceneState, now) ==
            L"Recording starts in 2s",
        "status message shows recording delay"
    );
    state.recordingDelayActive = false;

    state.recordingError = "disk full";
    require(
        ovtr::win32::makeStatusBarMessage(state, now) == L"Recording error: disk full",
        "recording error has status priority"
    );
    state.recordingError.clear();
    state.exportStatusMessage = "saved";
    require(
        ovtr::win32::makeStatusBarMessage(state, now) == L"Export: saved",
        "export status message"
    );
    state.exportStatusMessage.clear();
    state.status.runtimeInstalled = true;
    state.status.hmdPresent = false;
    require(
        ovtr::win32::makeStatusBarMessage(state, now) == L"Runtime detected. No HMD currently reported",
        "status reports missing HMD"
    );

    state.posePollFps = 12.25;
    state.renderFps = 45.5;
    state.targetViewportFps = 90.0;
    state.recordingDroppedFrames = 7;
    require(
        ovtr::win32::makeStatusBarMetrics(runtimeState, recordingState, viewportState) ==
            L"Pose FPS 12.2   View FPS 45.5   Target 90.0   Rec Idle   Frames 0   Dropped 7",
        "status metrics text"
    );

    state.status.dllLoaded = true;
    state.status.runtimeInstalled = true;
    state.status.hmdPresent = true;
    state.providerError = "provider offline";
    state.recordSaveFormat = ovtr::win32::ExportFormat::Fbx;
    state.recordExportSampleRate = 72.0f;
    state.recordDelaySeconds = 1.5f;
    state.originEnabled = true;
    state.originOffset = {1.0f, 2.0f, 3.0f};
    state.originRotationDegrees = {4.0f, 5.0f, 6.0f};
    state.originStatusMessage = "origin ready";
    state.importStatusMessage = "import ready";

    const std::vector<std::wstring> lines = ovtr::win32::makeDebugMonitorLines(
        runtimeState,
        recordingState,
        originState,
        importedSceneState,
        viewportState,
        now
    );
    require(!lines.empty() && lines.front() == L"Debug Monitor", "debug monitor title");
    bool foundProviderError = false;
    bool foundRecordingLine = false;
    bool foundOriginLine = false;
    bool foundImportStatus = false;
    for (const std::wstring& line : lines) {
        foundProviderError = foundProviderError || line == L"Provider error: provider offline";
        foundRecordingLine = foundRecordingLine || line.find(L"Save: fbx") != std::wstring::npos;
        foundOriginLine = foundOriginLine || line.find(L"Origin: Enabled") != std::wstring::npos;
        foundImportStatus = foundImportStatus || line == L"Import status: import ready";
    }
    require(foundProviderError, "debug monitor provider error line");
    require(foundRecordingLine, "debug monitor recording line");
    require(foundOriginLine, "debug monitor origin line");
    require(foundImportStatus, "debug monitor import status line");

    std::vector<std::wstring> narrowLines;
    ovtr::win32::appendDebugMonitorRecordingLines(narrowLines, recordingState, now);
    ovtr::win32::appendDebugMonitorOriginLines(narrowLines, runtimeState, originState);
    ovtr::win32::appendDebugMonitorImportLines(narrowLines, recordingState, importedSceneState);
    require(narrowLines.size() >= 3, "debug monitor narrow state line helpers");
}

} // namespace ovtr::test
