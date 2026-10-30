#include "platform/win32/VideoPlaybackService.h"
#include "platform/win32/VideoPlaybackServiceInternal.h"

#include <mfapi.h>
#include <mferror.h>

#include <cmath>

namespace ovtr::win32 {

std::string videoHresultMessage(const char* operation, HRESULT result);

VideoPlaybackService::VideoPlaybackService() : implementation_(std::make_unique<Implementation>())
{
    implementation_->startupResult = MFStartup(MF_VERSION, MFSTARTUP_FULL);
}

VideoPlaybackService::~VideoPlaybackService()
{
    close();
    if (SUCCEEDED(implementation_->startupResult)) {
        MFShutdown();
    }
}

bool VideoPlaybackService::open(const std::filesystem::path& path, std::string& error)
{
    close();
    if (FAILED(implementation_->startupResult)) {
        error = videoHresultMessage("Starting video playback runtime", implementation_->startupResult);
        return false;
    }
    std::error_code filesystemError;
    if (!std::filesystem::is_regular_file(path, filesystemError)) {
        error = filesystemError
            ? "Inspecting session video failed: " + filesystemError.message()
            : "Session video file is missing.";
        return false;
    }
    implementation_->requestedPath = path;
    implementation_->storeError({});
    implementation_->stopRequested.store(false, std::memory_order_release);
    implementation_->playbackState.store(VideoPlaybackState::Opening, std::memory_order_release);
    try {
        implementation_->worker = std::thread([implementation = implementation_.get()]() {
            implementation->runWorker();
        });
    } catch (const std::exception& exception) {
        implementation_->reader.Reset();
        implementation_->playbackState.store(VideoPlaybackState::Failed, std::memory_order_release);
        error = "Starting video playback worker failed: " + std::string(exception.what());
        return false;
    }
    requestFrame(0.0);
    error.clear();
    return true;
}

void VideoPlaybackService::close() noexcept
{
    implementation_->playbackState.store(VideoPlaybackState::Closed, std::memory_order_release);
    implementation_->stopRequested.store(true, std::memory_order_release);
    implementation_->requestCondition.notify_all();
    Microsoft::WRL::ComPtr<IMFSourceReader> reader;
    {
        std::lock_guard<std::mutex> lock(implementation_->readerMutex);
        reader = implementation_->reader;
    }
    if (reader) {
        const HRESULT flushResult = reader->Flush(MF_SOURCE_READER_ALL_STREAMS);
        if (FAILED(flushResult) && flushResult != MF_E_SHUTDOWN) {
            implementation_->storeError(
                videoHresultMessage("Canceling pending session video decode", flushResult));
        }
    }
    if (implementation_->worker.joinable()) {
        implementation_->worker.join();
    }
    {
        std::lock_guard<std::mutex> lock(implementation_->readerMutex);
        implementation_->reader.Reset();
    }
    {
        std::lock_guard<std::mutex> lock(implementation_->frameMutex);
        implementation_->frame.reset();
    }
    implementation_->lastDecodedSeconds = -1.0;
    implementation_->durationSeconds = 0.0;
    implementation_->reachedEndOfStream = false;
    implementation_->playbackState.store(VideoPlaybackState::Closed, std::memory_order_release);
}

void VideoPlaybackService::requestFrame(const double timeSeconds) noexcept
{
    const VideoPlaybackState current =
        implementation_->playbackState.load(std::memory_order_acquire);
    if (current != VideoPlaybackState::Opening &&
        current != VideoPlaybackState::Ready) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(implementation_->requestMutex);
        const double requested = timeSeconds > 0.0 ? timeSeconds : 0.0;
        if (implementation_->requestSequence > 0 &&
            std::abs(requested - implementation_->requestedSeconds) < (1.0 / 60.0)) {
            return;
        }
        implementation_->requestedSeconds = requested;
        ++implementation_->requestSequence;
    }
    implementation_->requestCondition.notify_one();
}

VideoFrameSnapshot VideoPlaybackService::latestFrame() const
{
    std::lock_guard<std::mutex> lock(implementation_->frameMutex);
    return implementation_->frame;
}

std::string VideoPlaybackService::runtimeError() const
{
    std::lock_guard<std::mutex> lock(implementation_->errorMutex);
    return implementation_->error;
}

bool VideoPlaybackService::opened() const noexcept
{
    return implementation_->playbackState.load(std::memory_order_acquire) ==
        VideoPlaybackState::Ready;
}

VideoPlaybackState VideoPlaybackService::state() const noexcept
{
    return implementation_->playbackState.load(std::memory_order_acquire);
}

void VideoPlaybackService::Implementation::storeError(std::string message)
{
    std::lock_guard<std::mutex> lock(errorMutex);
    error = std::move(message);
}

} // namespace ovtr::win32
