#pragma once

#include "platform/win32/ConfigTypes.h"

#include <chrono>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct AppRecordingState;
struct AppImportedSceneState;
struct AppOriginState;
struct AppRuntimeState;
struct AppViewportState;
struct AppWindowState;

std::wstring exportFormatDisplayText(ExportFormat format);

int remainingRecordDelaySeconds(
    const AppRecordingState& state,
    std::chrono::steady_clock::time_point now
) noexcept;
int remainingRecordDelaySeconds(const AppRecordingState& state) noexcept;
int remainingRecordDelaySeconds(
    const AppWindowState& state,
    std::chrono::steady_clock::time_point now
) noexcept;
int remainingRecordDelaySeconds(const AppWindowState& state) noexcept;

std::vector<std::wstring> makeDebugMonitorLines(
    const AppRuntimeState& runtimeState,
    const AppRecordingState& recordingState,
    const AppOriginState& originState,
    const AppImportedSceneState& importedSceneState,
    const AppViewportState& viewportState,
    std::chrono::steady_clock::time_point now
);
std::vector<std::wstring> makeDebugMonitorLines(
    const AppRuntimeState& runtimeState,
    const AppRecordingState& recordingState,
    const AppOriginState& originState,
    const AppImportedSceneState& importedSceneState,
    const AppViewportState& viewportState
);
std::vector<std::wstring> makeDebugMonitorLines(
    const AppWindowState& state,
    std::chrono::steady_clock::time_point now
);
std::vector<std::wstring> makeDebugMonitorLines(const AppWindowState& state);

std::wstring makeStatusBarMessage(
    const AppRuntimeState& runtimeState,
    const AppRecordingState& recordingState,
    const AppImportedSceneState& importedSceneState,
    std::chrono::steady_clock::time_point now
);
std::wstring makeStatusBarMessage(
    const AppRuntimeState& runtimeState,
    const AppRecordingState& recordingState,
    const AppImportedSceneState& importedSceneState
);
std::wstring makeStatusBarMessage(
    const AppWindowState& state,
    std::chrono::steady_clock::time_point now
);
std::wstring makeStatusBarMessage(const AppWindowState& state);

std::wstring makeStatusBarMetrics(
    const AppRuntimeState& runtimeState,
    const AppRecordingState& recordingState,
    const AppViewportState& viewportState
);
std::wstring makeStatusBarMetrics(const AppWindowState& state);

} // namespace ovtr::win32
