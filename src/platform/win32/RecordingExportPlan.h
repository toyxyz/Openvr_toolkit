#pragma once

#include "data/SessionTypes.h"
#include "export/ExportPoseTrack.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct AppDeviceState;
struct AppMarkerState;
struct AppRecordingState;
struct AppRuntimeState;

struct RecordingExportPlan {
    ovtr::RecordingSession session;
    std::vector<ovtr::ExportStaticPoseTrack> staticTracks;
    std::filesystem::path exportDirectory;
    double exportSampleRate = 60.0;
};

RecordingExportPlan makeRecordingExportPlan(
    const AppRecordingState& recordingState,
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState
);
RecordingExportPlan makeRecordingExportPlan(
    const AppRecordingState& recordingState,
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState,
    const std::wstring& sessionName
);
RecordingExportPlan makeRecordingExportPlan(
    const AppRecordingState& recordingState,
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState,
    const AppMarkerState& markerState,
    const std::wstring& sessionName
);
std::wstring sanitizedSessionFolderName(std::wstring sessionName);
std::filesystem::path sessionExportDirectoryPath(
    const std::filesystem::path& exportDirectory,
    const std::wstring& sessionName
);

} // namespace ovtr::win32
