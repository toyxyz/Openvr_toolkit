#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/SessionSkeletonExportClip.h"
#include "platform/win32/SkeletonRecordingTypes.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct AppWindowState;
struct MappingActor;

const MappingActor* loadedSessionSkeletonExportActor(const AppWindowState& state) noexcept;
const MappingActor* sessionSkeletonExportActor(const AppWindowState& state) noexcept;
std::vector<SessionSkeletonClipRequest> sessionSkeletonClipRequests(
    const AppWindowState& state,
    const ovtr::RecordingSession& session
);
std::filesystem::path loadedSessionExportDirectory(const AppWindowState& state);
bool buildLoadedSessionSkeletonClip(
    AppWindowState& state,
    SkeletonRecordingClip& clip,
    std::string& error
);
void exportLoadedSession(HWND hwnd, AppWindowState& state);

} // namespace ovtr::win32
