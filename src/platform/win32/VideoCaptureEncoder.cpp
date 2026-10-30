#include "platform/win32/VideoCaptureService.h"
#include "platform/win32/VideoCaptureServiceInternal.h"

#include <mfapi.h>

#include <cstring>
#include <limits>

namespace ovtr::win32 {
namespace {

constexpr std::size_t kMaximumQueuedFrames = 6;

HRESULT writeFrame(
    IMFSinkWriter* writer,
    const DWORD stream,
    const VideoFrame& frame,
    const LONGLONG sampleTime,
    const LONGLONG duration
)
{
    if (frame.bgra.size() > (std::numeric_limits<DWORD>::max)()) return E_INVALIDARG;
    const DWORD byteCount = static_cast<DWORD>(frame.bgra.size());
    Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
    HRESULT result = MFCreateMemoryBuffer(byteCount, &buffer);
    BYTE* destination = nullptr;
    if (SUCCEEDED(result)) result = buffer->Lock(&destination, nullptr, nullptr);
    if (SUCCEEDED(result)) {
        if (destination) {
            std::memcpy(destination, frame.bgra.data(), frame.bgra.size());
        } else {
            result = E_POINTER;
        }
        buffer->Unlock();
        if (SUCCEEDED(result)) result = buffer->SetCurrentLength(byteCount);
    }
    Microsoft::WRL::ComPtr<IMFSample> sample;
    if (SUCCEEDED(result)) result = MFCreateSample(&sample);
    if (SUCCEEDED(result)) result = sample->AddBuffer(buffer.Get());
    if (SUCCEEDED(result)) result = sample->SetSampleTime(sampleTime);
    if (SUCCEEDED(result)) result = sample->SetSampleDuration(duration);
    if (SUCCEEDED(result)) result = writer->WriteSample(stream, sample.Get());
    return result;
}

} // namespace

void VideoCaptureService::Implementation::enqueueRecordingFrame(VideoFrameSnapshot snapshot)
{
    if (!recordingActive.load(std::memory_order_acquire)) return;
    {
        std::lock_guard<std::mutex> lock(encoderMutex);
        if (!recordingActive.load(std::memory_order_relaxed)) return;
        if (encoderFrames.size() >= kMaximumQueuedFrames) {
            encoderFrames.pop_front();
            encoderDroppedFrames.fetch_add(1, std::memory_order_relaxed);
        }
        encoderFrames.push_back(std::move(snapshot));
    }
    encoderCondition.notify_one();
}

void VideoCaptureService::Implementation::runEncoder() noexcept
{
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
        const std::string error =
            videoHresultMessage("Initializing video encoder COM worker", comResult);
        storeError(error);
        {
            std::lock_guard<std::mutex> lock(encoderMutex);
            encoderOperationError = error;
        }
        recordingRequested.store(false, std::memory_order_release);
        if (preparationState.load(std::memory_order_acquire) ==
            VideoRecordingPreparationState::Pending) {
            preparationState.store(
                VideoRecordingPreparationState::Failed, std::memory_order_release);
        }
        encoderExited.store(true, std::memory_order_release);
        encoderFinishedCondition.notify_all();
        return;
    }
    for (;;) {
        VideoFrameSnapshot nextFrame;
        bool start = false;
        bool finish = false;
        bool stop = false;
        std::filesystem::path path;
        std::deque<VideoFrameSnapshot> finishingFrames;
        {
            std::unique_lock<std::mutex> lock(encoderMutex);
            encoderCondition.wait(lock, [this]() {
                return encoderStopRequested || encoderStartRequested ||
                    encoderFinishRequested || !encoderFrames.empty();
            });
            stop = encoderStopRequested;
            if (stop && !writer) break;
            start = encoderStartRequested;
            encoderStartRequested = false;
            finish = encoderFinishRequested;
            encoderFinishRequested = false;
            if (start) path = requestedRecordingPath;
            if (finish) {
                finishingFrames.swap(encoderFrames);
            } else if (!encoderFrames.empty()) {
                nextFrame = std::move(encoderFrames.front());
                encoderFrames.pop_front();
            }
        }
        std::string operationError;
        if (finish) {
            std::lock_guard<std::mutex> lock(encoderMutex);
            operationError = encoderOperationError;
        }
        if (start) {
            if (!createWriter(path, operationError)) {
                storeError(operationError);
                std::lock_guard<std::mutex> lock(encoderMutex);
                encoderOperationError = operationError;
                recordingRequested.store(false, std::memory_order_release);
                preparationState.store(
                    VideoRecordingPreparationState::Failed,
                    std::memory_order_release);
            } else {
                preparationState.store(
                    VideoRecordingPreparationState::Ready,
                    std::memory_order_release);
            }
        }
        const auto encode = [this, &operationError](const VideoFrameSnapshot& frame) {
            if (!frame || !writer) return;
            const LONGLONG duration = 10'000'000LL * frameRateDenominator /
                static_cast<LONGLONG>(frameRateNumerator);
            if (recordingEpochTimestamp100ns < 0 ||
                frame->timestamp100ns < recordingEpochTimestamp100ns) {
                operationError = "Camera supplied an invalid recording timestamp.";
                storeError(operationError);
                recordingActive.store(false, std::memory_order_release);
                std::lock_guard<std::mutex> lock(encoderMutex);
                encoderOperationError = operationError;
                return;
            }
            const LONGLONG sampleTime =
                frame->timestamp100ns - recordingEpochTimestamp100ns;
            if (recordingLastSampleTime100ns >= 0 &&
                sampleTime <= recordingLastSampleTime100ns) {
                operationError = "Camera recording timestamps are not increasing.";
                storeError(operationError);
                recordingActive.store(false, std::memory_order_release);
                std::lock_guard<std::mutex> lock(encoderMutex);
                encoderOperationError = operationError;
                return;
            }
            const HRESULT result = writeFrame(
                writer.Get(), writerStream, *frame, sampleTime, duration);
            if (FAILED(result)) {
                operationError = videoHresultMessage("Writing video frame", result);
                storeError(operationError);
                recordingActive.store(false, std::memory_order_release);
                std::lock_guard<std::mutex> lock(encoderMutex);
                encoderOperationError = operationError;
            } else {
                recordingLastSampleTime100ns = sampleTime;
            }
        };
        if (nextFrame && recordingActive.load(std::memory_order_acquire)) {
            encode(nextFrame);
        }
        for (const VideoFrameSnapshot& pending : finishingFrames) {
            if (!operationError.empty()) break;
            encode(pending);
        }
        if (finish || (stop && writer)) {
            if (!finishWriter(operationError)) storeError(operationError);
            std::lock_guard<std::mutex> lock(encoderMutex);
            encoderFrames.clear();
            if (operationError.empty() && !encoderOperationError.empty()) {
                operationError = encoderOperationError;
            }
            encoderOperationError = std::move(operationError);
            finalizationState.store(
                encoderOperationError.empty()
                    ? VideoFinalizationState::Succeeded
                    : VideoFinalizationState::Failed,
                std::memory_order_release);
            ++encoderFinishSequence;
            encoderFinishedCondition.notify_all();
        }
    }
    if (SUCCEEDED(comResult)) CoUninitialize();
    encoderExited.store(true, std::memory_order_release);
    encoderFinishedCondition.notify_all();
}

} // namespace ovtr::win32
