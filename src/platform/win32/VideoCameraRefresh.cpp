#include "platform/win32/VideoCameraRefresh.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/VideoCameraCatalog.h"

#include <objbase.h>

#include <algorithm>
#include <chrono>
#include <exception>

namespace ovtr::win32 {
namespace {

bool ready(std::future_status status) noexcept
{
    return status == std::future_status::ready;
}

bool cameraChangeRunning(const AppWindowState& state)
{
    // A ready future still owns the resulting capture service until the UI poll
    // consumes it. Replacing that future here would discard the completed state.
    return state.videoCaptureSwitch.valid();
}

VideoCameraCatalogResult enumerateOnComWorker()
{
    VideoCameraCatalogResult result;
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
        result.error = "Initializing camera refresh worker failed.";
        return result;
    }
    result = enumerateVideoCameras();
    if (SUCCEEDED(comResult)) CoUninitialize();
    return result;
}

void applyCameraList(AppWindowState& state, VideoCameraCatalogResult result)
{
    if (!result.success) {
        state.videoStatusMessage = result.error;
        appendDebugLog(state, "Camera list refresh failed: " + result.error);
        return;
    }
    state.videoCameras = std::move(result.cameras);
    state.selectedVideoCamera = -1;
    for (int index = 0; index < static_cast<int>(state.videoCameras.size()); ++index) {
        if (state.videoCameras[index].symbolicLink == state.selectedVideoCameraLink) {
            state.selectedVideoCamera = index;
            break;
        }
    }
    state.videoStatusMessage = state.videoCameras.empty()
        ? "No camera found."
        : std::to_string(state.videoCameras.size()) + " camera(s) found.";
}

void restoreCompletedSelection(
    AppWindowState& state,
    const VideoCameraInfo& completedCamera)
{
    state.selectedVideoCamera = -1;
    state.selectedVideoCameraLink = completedCamera.symbolicLink;
    state.selectedVideoCameraName = completedCamera.displayName;
    for (int index = 0; index < static_cast<int>(state.videoCameras.size()); ++index) {
        if (state.videoCameras[index].symbolicLink == completedCamera.symbolicLink) {
            state.selectedVideoCamera = index;
            break;
        }
    }
}

} // namespace

void requestVideoCameraRefresh(AppWindowState& state)
{
    if (state.videoCameraRefresh.valid() &&
        !ready(state.videoCameraRefresh.wait_for(std::chrono::seconds(0)))) {
        state.videoStatusMessage = "Camera refresh is already running.";
        return;
    }
    state.videoStatusMessage = "Refreshing cameras...";
    try {
        state.videoCameraRefresh = std::async(std::launch::async, enumerateOnComWorker);
    } catch (const std::exception& exception) {
        state.videoStatusMessage =
            "Starting camera refresh failed: " + std::string(exception.what());
        appendDebugLog(state, state.videoStatusMessage);
    }
}

bool requestVideoCameraSwitch(AppWindowState& state, const VideoCameraInfo& camera)
{
    if (cameraChangeRunning(state)) {
        state.videoCameraChangeQueued = true;
        state.queuedVideoCamera = camera;
        state.videoStatusMessage = "Camera activation queued...";
        return true;
    }
    std::shared_ptr<VideoCaptureService> previous = state.videoCapture;
    state.videoStatusMessage = "Switching camera...";
    try {
        state.videoCaptureSwitch = std::async(std::launch::async, [previous, camera]() {
            const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            VideoCaptureSwitchResult result;
            result.camera = camera;
            if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
                result.error = "Initializing camera activation worker failed.";
                return result;
            }
            result.capture = previous ? previous : std::make_shared<VideoCaptureService>();
            if (!result.capture->startPreview(camera.symbolicLink, result.error)) {
                result.capture.reset();
            }
            if (SUCCEEDED(comResult)) CoUninitialize();
            return result;
        });
    } catch (const std::exception& exception) {
        state.videoStatusMessage =
            "Starting camera activation failed: " + std::string(exception.what());
        appendDebugLog(state, state.videoStatusMessage);
        return false;
    }
    return true;
}

bool requestVideoCameraDisable(AppWindowState& state)
{
    if (cameraChangeRunning(state)) {
        state.videoCameraChangeQueued = true;
        state.queuedVideoCamera = {};
        state.videoStatusMessage = "Camera disable queued...";
        return true;
    }
    std::shared_ptr<VideoCaptureService> capture = state.videoCapture;
    state.videoStatusMessage = "Disabling video...";
    try {
        state.videoCaptureSwitch = std::async(std::launch::async, [capture]() {
            const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            VideoCaptureSwitchResult result;
            if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
                result.error = "Initializing camera disable worker failed.";
                return result;
            }
            if (capture) capture->stopPreview();
            result.capture = capture ? capture : std::make_shared<VideoCaptureService>();
            if (SUCCEEDED(comResult)) CoUninitialize();
            return result;
        });
    } catch (const std::exception& exception) {
        state.videoStatusMessage =
            "Starting camera disable failed: " + std::string(exception.what());
        appendDebugLog(state, state.videoStatusMessage);
        return false;
    }
    return true;
}

void pollVideoCameraTasks(HWND hwnd, AppWindowState& state)
{
    bool changed = false;
    if (state.videoCameraRefresh.valid() &&
        ready(state.videoCameraRefresh.wait_for(std::chrono::seconds(0)))) {
        try {
            applyCameraList(state, state.videoCameraRefresh.get());
        } catch (const std::exception& exception) {
            state.videoStatusMessage =
                "Camera refresh worker failed: " + std::string(exception.what());
            appendDebugLog(state, state.videoStatusMessage);
        }
        changed = true;
    }
    if (state.videoCaptureSwitch.valid() &&
        ready(state.videoCaptureSwitch.wait_for(std::chrono::seconds(0)))) {
        VideoCaptureSwitchResult result;
        try {
            result = state.videoCaptureSwitch.get();
        } catch (const std::exception& exception) {
            result.error =
                "Camera activation worker failed: " + std::string(exception.what());
        }
        const VideoCameraInfo completedCamera =
            result.capture ? result.camera : VideoCameraInfo{};
        if (result.capture) {
            state.videoCapture = std::move(result.capture);
            state.videoStatusMessage = result.camera.symbolicLink.empty()
                ? "Video disabled." : "Opening selected camera...";
        } else {
            state.videoStatusMessage = result.error.empty()
                ? "Starting camera preview failed." : result.error;
            appendDebugLog(state, "Camera preview start failed: " + state.videoStatusMessage);
            if (!state.videoCameraChangeQueued &&
                (result.camera.symbolicLink.empty() ||
                 state.selectedVideoCameraLink == result.camera.symbolicLink)) {
                state.selectedVideoCamera = -1;
                state.selectedVideoCameraLink.clear();
                state.selectedVideoCameraName.clear();
            }
        }
        if (state.videoCameraChangeQueued) {
            const VideoCameraInfo queuedCamera = std::move(state.queuedVideoCamera);
            state.videoCameraChangeQueued = false;
            state.queuedVideoCamera = {};
            const bool started = queuedCamera.symbolicLink.empty()
                ? requestVideoCameraDisable(state)
                : requestVideoCameraSwitch(state, queuedCamera);
            if (!started) restoreCompletedSelection(state, completedCamera);
        }
        changed = true;
    }
    if (changed) InvalidateRect(hwnd, nullptr, FALSE);
}

} // namespace ovtr::win32
