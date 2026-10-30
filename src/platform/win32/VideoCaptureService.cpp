#include "platform/win32/VideoCaptureService.h"
#include "platform/win32/VideoCaptureServiceInternal.h"
#include <mfapi.h>
#include <mferror.h>

#include <sstream>
#include <utility>

namespace ovtr::win32 {

std::string videoHresultMessage(const char* operation, const HRESULT result)
{
    std::ostringstream stream;
    stream << operation << " failed (HRESULT 0x" << std::hex
           << static_cast<unsigned long>(result) << ')';
    return stream.str();
}

VideoCaptureService::VideoCaptureService() : implementation_(std::make_unique<Implementation>())
{
    implementation_->startupResult = MFStartup(MF_VERSION, MFSTARTUP_FULL);
}

VideoCaptureService::~VideoCaptureService()
{
    stopPreview();
    if (SUCCEEDED(implementation_->startupResult)) {
        MFShutdown();
    }
}

bool VideoCaptureService::startPreview(const std::wstring& symbolicLink, std::string& error)
{
    stopPreview();
    if (FAILED(implementation_->startupResult)) {
        error = videoHresultMessage("Starting Media Foundation", implementation_->startupResult);
        return false;
    }
    if (symbolicLink.empty()) {
        error = "Select a camera before starting video preview.";
        return false;
    }
    implementation_->storeError({});
    implementation_->preparationState.store(
        VideoRecordingPreparationState::Idle, std::memory_order_release);
    implementation_->requestedCameraLink = symbolicLink;
    implementation_->stopRequested.store(false, std::memory_order_release);
    implementation_->previewActive.store(false, std::memory_order_release);
    implementation_->previewState.store(VideoPreviewState::Opening, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(implementation_->encoderMutex);
        implementation_->encoderStopRequested = false;
        implementation_->encoderStartRequested = false;
        implementation_->encoderFinishRequested = false;
        implementation_->encoderFrames.clear();
    }
    try {
        implementation_->encoderExited.store(false, std::memory_order_release);
        implementation_->encoderWorker = std::thread([implementation = implementation_.get()]() {
            implementation->runEncoder();
        });
        implementation_->worker = std::thread([implementation = implementation_.get()]() {
            implementation->runWorker();
        });
    } catch (const std::exception& exception) {
        implementation_->stopRequested.store(true, std::memory_order_release);
        {
            std::lock_guard<std::mutex> lock(implementation_->encoderMutex);
            implementation_->encoderStopRequested = true;
        }
        implementation_->encoderCondition.notify_all();
        if (implementation_->encoderWorker.joinable()) implementation_->encoderWorker.join();
        implementation_->encoderExited.store(true, std::memory_order_release);
        implementation_->previewActive.store(false, std::memory_order_release);
        implementation_->previewState.store(VideoPreviewState::Failed, std::memory_order_release);
        implementation_->reader.Reset();
        implementation_->mediaSource.Reset();
        error = "Starting video capture worker failed: " + std::string(exception.what());
        return false;
    }
    error.clear();
    return true;
}

void VideoCaptureService::stopPreview() noexcept
{
    implementation_->stopRequested.store(true, std::memory_order_release);
    Microsoft::WRL::ComPtr<IMFSourceReader> reader;
    Microsoft::WRL::ComPtr<IMFMediaSource> mediaSource;
    {
        std::lock_guard<std::mutex> lock(implementation_->sourceMutex);
        reader = implementation_->reader;
        mediaSource = implementation_->mediaSource;
    }
    if (reader) {
        const HRESULT flushResult = reader->Flush(MF_SOURCE_READER_ALL_STREAMS);
        if (FAILED(flushResult) && flushResult != MF_E_SHUTDOWN) {
            implementation_->storeError(
                videoHresultMessage("Canceling pending camera read", flushResult));
        }
    }
    if (implementation_->worker.joinable()) {
        implementation_->worker.join();
    }
    if (mediaSource) {
        const HRESULT shutdownResult = mediaSource->Shutdown();
        if (FAILED(shutdownResult) && shutdownResult != MF_E_SHUTDOWN) {
            implementation_->storeError(
                videoHresultMessage("Shutting down camera source", shutdownResult));
        }
    }
    std::string finalizeError;
    if (recording() && !endRecording(finalizeError)) {
        implementation_->storeError(finalizeError);
    }
    {
        std::lock_guard<std::mutex> lock(implementation_->encoderMutex);
        implementation_->encoderStopRequested = true;
    }
    implementation_->encoderCondition.notify_all();
    if (implementation_->encoderWorker.joinable()) {
        implementation_->encoderWorker.join();
    }
    {
        std::lock_guard<std::mutex> lock(implementation_->sourceMutex);
        implementation_->reader.Reset();
        implementation_->mediaSource.Reset();
    }
    {
        std::lock_guard<std::mutex> lock(implementation_->frameMutex);
        implementation_->frame.reset();
    }
    implementation_->previewActive.store(false, std::memory_order_release);
    implementation_->previewState.store(VideoPreviewState::Stopped, std::memory_order_release);
}

bool VideoCaptureService::beginRecording(const std::filesystem::path& outputPath, std::string& error)
{
    const VideoPreviewState currentPreview =
        implementation_->previewState.load(std::memory_order_acquire);
    if (currentPreview != VideoPreviewState::Ready || !previewing()) {
        error = currentPreview == VideoPreviewState::Opening
            ? "The selected camera is still opening."
            : "The selected camera is not ready.";
        return false;
    }
    if (implementation_->encoderExited.load(std::memory_order_acquire)) {
        error = "Video encoder worker is not running.";
        return false;
    }
    if (recording()) {
        error = "Video recording is already active.";
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(implementation_->encoderMutex);
        implementation_->requestedRecordingPath = outputPath;
        implementation_->encoderOperationError.clear();
        implementation_->encoderDroppedFrames.store(0, std::memory_order_release);
        implementation_->encoderStartRequested = true;
    }
    implementation_->finalizationState.store(VideoFinalizationState::Idle, std::memory_order_release);
    implementation_->preparationState.store(
        VideoRecordingPreparationState::Pending, std::memory_order_release);
    implementation_->recordingRequested.store(true, std::memory_order_release);
    implementation_->encoderCondition.notify_one();
    error.clear();
    return true;
}

bool VideoCaptureService::activatePreparedRecording(std::string& error) noexcept
{
    if (implementation_->preparationState.load(std::memory_order_acquire) !=
        VideoRecordingPreparationState::Ready) {
        error = "Video encoder is not ready.";
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(implementation_->frameMutex);
        if (!implementation_->frame) {
            error = "Waiting for the first camera frame.";
            return false;
        }
        // Anchor the video clock immediately before the tracker clock starts.
        // The next captured frame then retains its real camera-clock offset.
        implementation_->recordingEpochTimestamp100ns =
            implementation_->frame->timestamp100ns;
        implementation_->recordingLastSampleTime100ns = -1;
    }
    implementation_->recordingActive.store(true, std::memory_order_release);
    implementation_->preparationState.store(
        VideoRecordingPreparationState::Idle, std::memory_order_release);
    error.clear();
    return true;
}

bool VideoCaptureService::requestEndRecording(std::string& error) noexcept
{
    if (implementation_->encoderExited.load(std::memory_order_acquire)) {
        error = "Video encoder worker is not running.";
        return false;
    }
    if (implementation_->finalizationState.load(std::memory_order_acquire) ==
        VideoFinalizationState::Pending) {
        error.clear();
        return true;
    }
    implementation_->recordingRequested.store(false, std::memory_order_release);
    implementation_->recordingActive.store(false, std::memory_order_release);
    implementation_->preparationState.store(
        VideoRecordingPreparationState::Idle, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(implementation_->encoderMutex);
        implementation_->encoderFinishRequested = true;
        implementation_->finalizationState.store(
            VideoFinalizationState::Pending, std::memory_order_release);
    }
    implementation_->encoderCondition.notify_one();
    error.clear();
    return true;
}

bool VideoCaptureService::endRecording(std::string& error) noexcept
{
    std::uint64_t pendingSequence = 0;
    {
        std::lock_guard<std::mutex> lock(implementation_->encoderMutex);
        pendingSequence = implementation_->encoderFinishSequence;
    }
    if (!requestEndRecording(error)) return false;
    std::unique_lock<std::mutex> lock(implementation_->encoderMutex);
    implementation_->encoderFinishedCondition.wait(lock, [implementation = implementation_.get(), pendingSequence]() {
        return implementation->encoderFinishSequence != pendingSequence ||
            implementation->encoderExited.load(std::memory_order_acquire);
    });
    error = implementation_->encoderOperationError;
    return error.empty();
}

VideoFrameSnapshot VideoCaptureService::latestFrame() const
{
    std::lock_guard<std::mutex> lock(implementation_->frameMutex);
    return implementation_->frame;
}

bool VideoCaptureService::previewing() const noexcept
{
    return implementation_->previewActive.load(std::memory_order_acquire);
}

VideoPreviewState VideoCaptureService::previewState() const noexcept
{
    return implementation_->previewState.load(std::memory_order_acquire);
}

bool VideoCaptureService::recording() const noexcept
{
    return implementation_->recordingRequested.load(std::memory_order_acquire) ||
        implementation_->recordingActive.load(std::memory_order_acquire);
}

std::uint64_t VideoCaptureService::droppedRecordingFrames() const noexcept
{
    return implementation_->encoderDroppedFrames.load(std::memory_order_acquire);
}

VideoRecordingPreparationState VideoCaptureService::preparationState() const noexcept
{
    return implementation_->preparationState.load(std::memory_order_acquire);
}

std::string VideoCaptureService::preparationError() const
{
    std::lock_guard<std::mutex> lock(implementation_->encoderMutex);
    return implementation_->encoderOperationError;
}

VideoFinalizationState VideoCaptureService::finalizationState() const noexcept
{
    return implementation_->finalizationState.load(std::memory_order_acquire);
}

std::string VideoCaptureService::finalizationError() const
{
    std::lock_guard<std::mutex> lock(implementation_->encoderMutex);
    return implementation_->encoderOperationError;
}

std::string VideoCaptureService::runtimeError() const
{
    std::lock_guard<std::mutex> lock(implementation_->errorMutex);
    return implementation_->error;
}

void VideoCaptureService::Implementation::storeError(std::string message)
{
    std::lock_guard<std::mutex> lock(errorMutex);
    error = std::move(message);
}
} // namespace ovtr::win32
