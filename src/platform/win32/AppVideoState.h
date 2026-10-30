#pragma once

#include "platform/win32/VideoCameraCatalog.h"
#include "platform/win32/VideoCaptureService.h"
#include "platform/win32/VideoPlaybackService.h"
#include "platform/win32/RecordingStartPlan.h"

#include <memory>
#include <future>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct VideoCaptureSwitchResult {
    std::shared_ptr<VideoCaptureService> capture;
    VideoCameraInfo camera;
    std::string error;
};

struct AppVideoState {
    bool videoPanelVisible = false;
    std::vector<VideoCameraInfo> videoCameras;
    int selectedVideoCamera = -1;
    std::wstring selectedVideoCameraLink;
    std::wstring selectedVideoCameraName;
    std::string videoStatusMessage;
    std::shared_ptr<VideoCaptureService> videoCapture = std::make_shared<VideoCaptureService>();
    std::shared_ptr<VideoPlaybackService> sessionVideoPlayback = std::make_shared<VideoPlaybackService>();
    std::future<VideoCameraCatalogResult> videoCameraRefresh;
    std::future<VideoCaptureSwitchResult> videoCaptureSwitch;
    bool videoCameraChangeQueued = false;
    VideoCameraInfo queuedVideoCamera;
    std::uint64_t reportedVideoDroppedFrames = 0;
    std::optional<RecordingStartPlan> pendingVideoRecordingStart;
    bool videoFinalizationPending = false;
    bool pendingTrackerStopSucceeded = false;
    bool pendingExportAfterRecording = false;
    std::uint64_t pendingStoppedFrameCount = 0;
    std::uint64_t pendingStoppedDroppedFrames = 0;
    std::string pendingTrackerStopError;
};

} // namespace ovtr::win32
