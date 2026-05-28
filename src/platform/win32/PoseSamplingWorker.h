#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "vr/IVRProvider.h"

#include <chrono>

namespace ovtr::win32 {

struct AppWindowState;

inline constexpr UINT kPoseSamplingStatusMessage = WM_APP + 1;
inline constexpr double kPoseSamplingTargetFps = 90.0;

void startPoseSamplingWorker(HWND hwnd, AppWindowState& state);
void stopPoseSamplingWorker(AppWindowState& state) noexcept;
ovtr::PosePollResult copyLatestPoseSnapshot(const AppWindowState& state);
void storeLatestPoseSnapshot(AppWindowState& state, ovtr::PosePollResult poses);
bool appendRecordingFrameIfDue(
    AppWindowState& state,
    const ovtr::PosePollResult& poses,
    std::chrono::steady_clock::time_point now
);
bool pollAndRecordPoseSampleOnce(
    AppWindowState& state,
    std::chrono::steady_clock::time_point now
);

} // namespace ovtr::win32
