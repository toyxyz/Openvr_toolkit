#include "platform/win32/StatusPanel.h"

#include "platform/win32/AppImportedSceneState.h"
#include "platform/win32/AppRecordingState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/AppViewportState.h"
#include "platform/win32/DebugPanel.h"
#include "platform/win32/Win32String.h"

#include <iomanip>
#include <mutex>
#include <sstream>

namespace ovtr::win32 {

std::wstring makeStatusBarMessage(
    const AppRuntimeState& runtimeState,
    const AppRecordingState& recordingState,
    const AppImportedSceneState& importedSceneState,
    const std::chrono::steady_clock::time_point now
)
{
    const ovtr::SteamVRRuntimeStatus& status = runtimeState.status;

    std::string recordingError;
    std::string exportStatusMessage;
    bool recordingDelayActive = false;
    int remainingDelaySeconds = 0;
    ovtr::RecorderState recorderState = ovtr::RecorderState::Idle;
    {
        std::lock_guard<std::mutex> lock(recordingState.recordingMutex);
        recordingError = recordingState.recordingError;
        exportStatusMessage = recordingState.exportStatusMessage;
        recordingDelayActive = recordingState.recordingDelayActive;
        remainingDelaySeconds = remainingRecordDelaySeconds(recordingState, now);
        recorderState = recordingState.recorder.state();
    }

    if (!recordingError.empty()) {
        return L"Recording error: " + widen(recordingError);
    }
    if (recordingDelayActive) {
        return L"Recording starts in " + std::to_wstring(remainingDelaySeconds) + L"s";
    }
    if (!exportStatusMessage.empty()) {
        return L"Export: " + widen(exportStatusMessage);
    }
    if (!importedSceneState.importStatusMessage.empty()) {
        return L"Import: " + widen(importedSceneState.importStatusMessage);
    }
    if (!runtimeState.providerError.empty()) {
        return L"OpenVR provider: " + widen(runtimeState.providerError);
    }
    if (!status.error.empty()) {
        return L"SteamVR: " + widen(status.error);
    }
    if (recorderState == ovtr::RecorderState::Recording) {
        return L"Recording session";
    }
    if (recorderState == ovtr::RecorderState::Finalizing) {
        return L"Finalizing recording";
    }
    if (!status.runtimeInstalled) {
        return L"SteamVR runtime was not detected";
    }
    if (!status.hmdPresent) {
        return L"Runtime detected. No HMD currently reported";
    }
    if (!runtimeState.provider.isInitialized()) {
        return L"OpenVR provider initializing";
    }

    return L"Runtime and HMD detected";
}

std::wstring makeStatusBarMessage(
    const AppRuntimeState& runtimeState,
    const AppRecordingState& recordingState,
    const AppImportedSceneState& importedSceneState
)
{
    return makeStatusBarMessage(
        runtimeState,
        recordingState,
        importedSceneState,
        std::chrono::steady_clock::now()
    );
}

std::wstring makeStatusBarMetrics(
    const AppRuntimeState& runtimeState,
    const AppRecordingState& recordingState,
    const AppViewportState& viewportState
)
{
    bool recordingDelayActive = false;
    ovtr::RecorderState recorderState = ovtr::RecorderState::Idle;
    std::uint64_t frameCount = 0;
    std::uint64_t droppedFrames = 0;
    {
        std::lock_guard<std::mutex> lock(recordingState.recordingMutex);
        recordingDelayActive = recordingState.recordingDelayActive;
        recorderState = recordingState.recorder.state();
        frameCount = recordingState.recorder.frameCount();
        droppedFrames = recordingState.recordingDroppedFrames;
    }

    const std::wstring recordingStatus = recordingDelayActive ? L"Waiting" : recorderStateText(recorderState);
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(1)
           << L"Pose FPS " << runtimeState.posePollFps
           << L"   View FPS " << runtimeState.renderFps
           << L"   Target " << viewportState.targetViewportFps
           << L"   Rec " << recordingStatus
           << L"   Frames " << frameCount
           << L"   Dropped " << droppedFrames;
    return stream.str();
}

} // namespace ovtr::win32
