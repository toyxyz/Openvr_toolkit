#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <chrono>
#include <filesystem>
#include <string>

namespace ovtr::win32 {

struct AppLoadedSessionState;
struct AppWindowState;

enum class SessionPlaybackToggleResult {
    Ignored,
    Started,
    Paused,
};

bool openLoadedSession(AppWindowState& state, const std::filesystem::path& folder, std::string& outError);
bool closeLoadedSession(AppWindowState& state);
double loadedSessionDurationSeconds(const AppLoadedSessionState& state) noexcept;
int loadedSessionTotalFrames(const AppLoadedSessionState& state) noexcept;
int loadedSessionCurrentFrame(const AppLoadedSessionState& state) noexcept;
double loadedSessionPlaybackTime(const AppLoadedSessionState& state) noexcept;
void setLoadedSessionPlaybackSeconds(
    AppLoadedSessionState& state,
    double playbackSeconds,
    std::chrono::steady_clock::time_point now
) noexcept;
void setLoadedSessionPlaybackSeconds(AppLoadedSessionState& state, double playbackSeconds) noexcept;
void updateLoadedSessionPlayback(
    AppLoadedSessionState& state,
    std::chrono::steady_clock::time_point now
) noexcept;
void updateLoadedSessionPlayback(AppLoadedSessionState& state) noexcept;
bool seekLoadedSessionFromTimeline(AppLoadedSessionState& state, const RECT& timelineRect, POINT point) noexcept;
SessionPlaybackToggleResult toggleLoadedSessionPlayback(
    AppLoadedSessionState& state,
    std::chrono::steady_clock::time_point now
) noexcept;
SessionPlaybackToggleResult toggleLoadedSessionPlayback(AppLoadedSessionState& state) noexcept;
bool sampleLoadedSessionFrame(AppWindowState& state);

} // namespace ovtr::win32
