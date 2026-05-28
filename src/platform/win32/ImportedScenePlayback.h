#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <chrono>

namespace ovtr::win32 {

struct AppImportedSceneState;
struct AppWindowState;

constexpr double kImportedAnimationFrameRate = 60.0;

enum class ImportedScenePlaybackToggleResult {
    Ignored,
    Started,
    Paused,
};

double importedSceneDurationSeconds(const AppImportedSceneState& state) noexcept;
int importedSceneTotalFrames(const AppImportedSceneState& state) noexcept;
int importedSceneCurrentFrame(const AppImportedSceneState& state) noexcept;
void setImportedScenePlaybackSeconds(
    AppImportedSceneState& state,
    double playbackSeconds,
    std::chrono::steady_clock::time_point now
) noexcept;
void setImportedScenePlaybackSeconds(AppImportedSceneState& state, double playbackSeconds) noexcept;
void updateImportedScenePlayback(
    AppImportedSceneState& state,
    std::chrono::steady_clock::time_point now
) noexcept;
void updateImportedScenePlayback(AppImportedSceneState& state) noexcept;
double importedScenePlaybackTime(const AppImportedSceneState& state) noexcept;
bool closeImportedScene(AppImportedSceneState& state);
bool seekImportedSceneFromTimeline(AppImportedSceneState& state, const RECT& timelineRect, POINT point) noexcept;
ImportedScenePlaybackToggleResult toggleImportedScenePlayback(
    AppImportedSceneState& state,
    std::chrono::steady_clock::time_point now
) noexcept;
ImportedScenePlaybackToggleResult toggleImportedScenePlayback(AppImportedSceneState& state) noexcept;
bool startImportedScenePlaybackForRecording(
    AppImportedSceneState& state,
    std::chrono::steady_clock::time_point now
) noexcept;
bool startImportedScenePlaybackForRecording(AppImportedSceneState& state) noexcept;

double importedSceneDurationSeconds(const AppWindowState& state) noexcept;
int importedSceneTotalFrames(const AppWindowState& state) noexcept;
int importedSceneCurrentFrame(const AppWindowState& state) noexcept;
void setImportedScenePlaybackSeconds(
    AppWindowState& state,
    double playbackSeconds,
    std::chrono::steady_clock::time_point now
) noexcept;
void setImportedScenePlaybackSeconds(AppWindowState& state, double playbackSeconds) noexcept;
void updateImportedScenePlayback(
    AppWindowState& state,
    std::chrono::steady_clock::time_point now
) noexcept;
void updateImportedScenePlayback(AppWindowState& state) noexcept;
double importedScenePlaybackTime(const AppWindowState& state) noexcept;
bool closeImportedScene(AppWindowState& state);
bool seekImportedSceneFromTimeline(AppWindowState& state, const RECT& timelineRect, POINT point) noexcept;
ImportedScenePlaybackToggleResult toggleImportedScenePlayback(
    AppWindowState& state,
    std::chrono::steady_clock::time_point now
) noexcept;
ImportedScenePlaybackToggleResult toggleImportedScenePlayback(AppWindowState& state) noexcept;
bool startImportedScenePlaybackForRecording(
    AppWindowState& state,
    std::chrono::steady_clock::time_point now
) noexcept;
bool startImportedScenePlaybackForRecording(AppWindowState& state) noexcept;

} // namespace ovtr::win32
