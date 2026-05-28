#include "platform/win32/StatusPanel.h"

#include "platform/win32/AppImportedSceneState.h"
#include "platform/win32/AppOriginState.h"
#include "platform/win32/AppRecordingState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/AppViewportState.h"
#include "platform/win32/DebugPanel.h"
#include "platform/win32/DebugMonitorStatusLines.h"
#include "platform/win32/Win32String.h"

#include <iomanip>
#include <sstream>

namespace ovtr::win32 {

std::vector<std::wstring> makeDebugMonitorLines(
    const AppRuntimeState& runtimeState,
    const AppRecordingState& recordingState,
    const AppOriginState& originState,
    const AppImportedSceneState& importedSceneState,
    const AppViewportState& viewportState,
    const std::chrono::steady_clock::time_point now
)
{
    const ovtr::SteamVRRuntimeStatus& status = runtimeState.status;
    std::vector<std::wstring> lines;
    lines.emplace_back(L"Debug Monitor");
    lines.emplace_back(L"SteamVR runtime DLL loaded: " + yesNo(status.dllLoaded));
    lines.emplace_back(L"OpenVR runtime installed: " + yesNo(status.runtimeInstalled));
    lines.emplace_back(L"HMD present: " + yesNo(status.hmdPresent));
    lines.emplace_back(L"OpenVR provider initialized: " + yesNo(runtimeState.provider.isInitialized()));
    lines.emplace_back(L"DLL path: " + widen(status.dllPath));
    lines.emplace_back(L"Runtime path: " + widen(status.runtimePath));

    if (!status.error.empty()) {
        lines.emplace_back(L"Runtime status: " + widen(status.error));
    } else if (!status.runtimeInstalled) {
        lines.emplace_back(L"Runtime status: SteamVR runtime was not detected.");
    } else if (!status.hmdPresent) {
        lines.emplace_back(L"Runtime status: Runtime detected. No HMD currently reported.");
    } else {
        lines.emplace_back(L"Runtime status: Runtime and HMD detected.");
    }

    if (!runtimeState.providerError.empty()) {
        lines.emplace_back(L"Provider error: " + widen(runtimeState.providerError));
    }

    {
        std::wostringstream stream;
        stream << std::fixed << std::setprecision(1)
               << L"Pose FPS: " << runtimeState.posePollFps
               << L"   View FPS: " << runtimeState.renderFps
               << L"   Target: " << viewportState.targetViewportFps
               << L"   Pose samples: " << runtimeState.poses.poses.size();
        lines.emplace_back(stream.str());
    }

    appendDebugMonitorRecordingLines(lines, recordingState, now);
    appendDebugMonitorOriginLines(lines, runtimeState, originState);
    appendDebugMonitorImportLines(lines, recordingState, importedSceneState);

    return lines;
}

std::vector<std::wstring> makeDebugMonitorLines(
    const AppRuntimeState& runtimeState,
    const AppRecordingState& recordingState,
    const AppOriginState& originState,
    const AppImportedSceneState& importedSceneState,
    const AppViewportState& viewportState
)
{
    return makeDebugMonitorLines(
        runtimeState,
        recordingState,
        originState,
        importedSceneState,
        viewportState,
        std::chrono::steady_clock::now()
    );
}

} // namespace ovtr::win32
