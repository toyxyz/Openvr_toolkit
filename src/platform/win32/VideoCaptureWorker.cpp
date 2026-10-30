#include "platform/win32/VideoCaptureService.h"
#include "platform/win32/VideoCaptureServiceInternal.h"

#include <mfapi.h>

#include <algorithm>
#include <cstring>

namespace ovtr::win32 {
namespace {

VideoFrameSnapshot copyFrame(
    IMFSample* sample,
    const std::uint32_t width,
    const std::uint32_t height,
    const std::uint64_t sequence,
    const LONGLONG timestamp,
    const std::int32_t signedStride
)
{
    Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
    if (FAILED(sample->ConvertToContiguousBuffer(&buffer))) {
        return {};
    }
    BYTE* bytes = nullptr;
    DWORD length = 0;
    const HRESULT lockResult = buffer->Lock(&bytes, nullptr, &length);
    if (FAILED(lockResult)) {
        return {};
    }
    if (!bytes) {
        buffer->Unlock();
        return {};
    }
    const std::size_t rowBytes = static_cast<std::size_t>(width) * 4u;
    const std::size_t sourceStride = signedStride < 0
        ? static_cast<std::size_t>(-static_cast<std::int64_t>(signedStride))
        : static_cast<std::size_t>(signedStride);
    const std::size_t sourceBytes = sourceStride * static_cast<std::size_t>(height);
    const std::size_t outputBytes = rowBytes * static_cast<std::size_t>(height);
    if (signedStride == 0 || sourceStride < rowBytes ||
        static_cast<std::size_t>(length) < sourceBytes) {
        buffer->Unlock();
        return {};
    }
    auto frame = std::make_shared<VideoFrame>();
    frame->width = width;
    frame->height = height;
    frame->stride = static_cast<std::uint32_t>(rowBytes);
    frame->sequence = sequence;
    frame->timestamp100ns = timestamp;
    frame->topDown = true;
    frame->bgra.resize(outputBytes);
    for (std::uint32_t row = 0; row < height; ++row) {
        const std::uint32_t sourceRow = signedStride > 0 ? row : height - 1u - row;
        std::memcpy(
            frame->bgra.data() + static_cast<std::size_t>(row) * rowBytes,
            bytes + static_cast<std::size_t>(sourceRow) * sourceStride,
            rowBytes);
    }
    buffer->Unlock();
    return frame;
}

} // namespace

void VideoCaptureService::Implementation::runWorker() noexcept
{
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
        storeError(videoHresultMessage("Initializing video capture COM worker", comResult));
        previewActive.store(false, std::memory_order_release);
        previewState.store(VideoPreviewState::Failed, std::memory_order_release);
        return;
    }
    std::string openError;
    if (!openCamera(requestedCameraLink, openError)) {
        if (!stopRequested.load(std::memory_order_acquire)) {
            storeError(openError);
            previewState.store(VideoPreviewState::Failed, std::memory_order_release);
        }
        if (SUCCEEDED(comResult)) {
            CoUninitialize();
        }
        return;
    }
    Microsoft::WRL::ComPtr<IMFSourceReader> workerReader;
    {
        std::lock_guard<std::mutex> lock(sourceMutex);
        workerReader = reader;
    }
    if (!workerReader) {
        storeError("Camera source reader was not published.");
        previewState.store(VideoPreviewState::Failed, std::memory_order_release);
        if (SUCCEEDED(comResult)) CoUninitialize();
        return;
    }
    previewActive.store(true, std::memory_order_release);
    previewState.store(VideoPreviewState::Ready, std::memory_order_release);
    while (!stopRequested.load(std::memory_order_acquire)) {
        DWORD flags = 0;
        LONGLONG timestamp = 0;
        Microsoft::WRL::ComPtr<IMFSample> sample;
        const HRESULT result = workerReader->ReadSample(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0,
            nullptr,
            &flags,
            &timestamp,
            &sample
        );
        if (FAILED(result)) {
            if (!stopRequested.load(std::memory_order_acquire)) {
                storeError(videoHresultMessage("Reading camera frame", result));
                previewState.store(VideoPreviewState::Failed, std::memory_order_release);
            }
            break;
        }
        if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) {
            storeError("Camera stream ended unexpectedly.");
            previewState.store(VideoPreviewState::Failed, std::memory_order_release);
            break;
        }
        if (!sample) {
            continue;
        }
        VideoFrameSnapshot snapshot = copyFrame(
            sample.Get(), width, height, ++frameSequence, timestamp, captureStride);
        if (!snapshot) {
            storeError("Copying camera frame failed.");
            previewState.store(VideoPreviewState::Failed, std::memory_order_release);
            break;
        }
        {
            std::lock_guard<std::mutex> lock(frameMutex);
            frame = snapshot;
        }
        enqueueRecordingFrame(std::move(snapshot));
    }
    previewActive.store(false, std::memory_order_release);
    if (SUCCEEDED(comResult)) {
        CoUninitialize();
    }
}

} // namespace ovtr::win32
