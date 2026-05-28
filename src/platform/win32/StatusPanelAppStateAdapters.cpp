#include "platform/win32/StatusPanel.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

int remainingRecordDelaySeconds(
    const AppWindowState& state,
    const std::chrono::steady_clock::time_point now
) noexcept
{
    return remainingRecordDelaySeconds(static_cast<const AppRecordingState&>(state), now);
}

int remainingRecordDelaySeconds(const AppWindowState& state) noexcept
{
    return remainingRecordDelaySeconds(static_cast<const AppRecordingState&>(state));
}

std::vector<std::wstring> makeDebugMonitorLines(
    const AppWindowState& state,
    const std::chrono::steady_clock::time_point now
)
{
    return makeDebugMonitorLines(
        static_cast<const AppRuntimeState&>(state),
        static_cast<const AppRecordingState&>(state),
        static_cast<const AppOriginState&>(state),
        static_cast<const AppImportedSceneState&>(state),
        static_cast<const AppViewportState&>(state),
        now
    );
}

std::vector<std::wstring> makeDebugMonitorLines(const AppWindowState& state)
{
    return makeDebugMonitorLines(
        static_cast<const AppRuntimeState&>(state),
        static_cast<const AppRecordingState&>(state),
        static_cast<const AppOriginState&>(state),
        static_cast<const AppImportedSceneState&>(state),
        static_cast<const AppViewportState&>(state)
    );
}

std::wstring makeStatusBarMessage(
    const AppWindowState& state,
    const std::chrono::steady_clock::time_point now
)
{
    return makeStatusBarMessage(
        static_cast<const AppRuntimeState&>(state),
        static_cast<const AppRecordingState&>(state),
        static_cast<const AppImportedSceneState&>(state),
        now
    );
}

std::wstring makeStatusBarMessage(const AppWindowState& state)
{
    return makeStatusBarMessage(
        static_cast<const AppRuntimeState&>(state),
        static_cast<const AppRecordingState&>(state),
        static_cast<const AppImportedSceneState&>(state)
    );
}

std::wstring makeStatusBarMetrics(const AppWindowState& state)
{
    return makeStatusBarMetrics(
        static_cast<const AppRuntimeState&>(state),
        static_cast<const AppRecordingState&>(state),
        static_cast<const AppViewportState&>(state)
    );
}

} // namespace ovtr::win32
