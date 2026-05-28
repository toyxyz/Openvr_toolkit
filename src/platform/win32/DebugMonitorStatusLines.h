#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct AppImportedSceneState;
struct AppOriginState;
struct AppRecordingState;
struct AppRuntimeState;
struct AppWindowState;

void appendDebugMonitorRecordingLines(
    std::vector<std::wstring>& lines,
    const AppRecordingState& state,
    std::chrono::steady_clock::time_point now
);
void appendDebugMonitorOriginLines(
    std::vector<std::wstring>& lines,
    const AppRuntimeState& runtimeState,
    const AppOriginState& originState
);
void appendDebugMonitorImportLines(
    std::vector<std::wstring>& lines,
    const AppRecordingState& recordingState,
    const AppImportedSceneState& importedSceneState
);

} // namespace ovtr::win32
