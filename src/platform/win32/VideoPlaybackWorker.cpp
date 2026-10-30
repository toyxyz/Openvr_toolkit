#include "platform/win32/VideoPlaybackService.h"
#include "platform/win32/VideoPlaybackServiceInternal.h"

#include <mfapi.h>
#include <mferror.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <propvarutil.h>

namespace ovtr::win32 {

std::string videoHresultMessage(const char* operation, HRESULT result);

void VideoPlaybackService::Implementation::decodeAt(const double seconds) noexcept
{
    const double lastPlayableSeconds = durationSeconds > 0.0
        ? (std::max)(0.0, durationSeconds - frameDurationSeconds)
        : 0.0;
    const double playableSeconds = durationSeconds > 0.0
        ? std::clamp(seconds, 0.0, lastPlayableSeconds)
        : (std::max)(seconds, 0.0);
    const double delta = lastDecodedSeconds >= 0.0
        ? playableSeconds - lastDecodedSeconds
        : 0.0;
    if (reachedEndOfStream && delta >= 0.0) {
        return;
    }
    if (lastDecodedSeconds >= 0.0 && std::abs(delta) < frameDurationSeconds * 0.5) {
        return;
    }
    const bool seekRequired = lastDecodedSeconds < 0.0 ||
        delta < 0.0 || delta > 0.5;
    HRESULT result = S_OK;
    if (seekRequired) {
        PROPVARIANT position;
        PropVariantInit(&position);
        position.vt = VT_I8;
        position.hVal.QuadPart =
            static_cast<LONGLONG>(playableSeconds * 10'000'000.0);
        result = reader->SetCurrentPosition(GUID_NULL, position);
        PropVariantClear(&position);
        if (FAILED(result)) {
            if (result == MF_E_INVALID_POSITION && durationSeconds <= 0.0) {
                reachedEndOfStream = true;
                lastDecodedSeconds = playableSeconds;
                return;
            }
            if (!stopRequested.load(std::memory_order_acquire)) {
                storeError(videoHresultMessage("Seeking session video", result));
            }
            return;
        }
        reachedEndOfStream = false;
    }

    Microsoft::WRL::ComPtr<IMFSample> selected;
    LONGLONG selectedTimestamp = 0;
    const LONGLONG target =
        static_cast<LONGLONG>(playableSeconds * 10'000'000.0);
    bool reachedEnd = false;
    for (int attempt = 0; attempt < 120; ++attempt) {
        if (stopRequested.load(std::memory_order_acquire)) return;
        DWORD flags = 0;
        Microsoft::WRL::ComPtr<IMFSample> sample;
        result = reader->ReadSample(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, &flags, &selectedTimestamp, &sample
        );
        if (FAILED(result)) break;
        if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) {
            reachedEnd = true;
            break;
        }
        if (sample) {
            selected = std::move(sample);
            if (selectedTimestamp >= target) break;
        }
    }
    if (FAILED(result)) {
        if (!stopRequested.load(std::memory_order_acquire)) {
            storeError(videoHresultMessage("Decoding session video", result));
        }
        return;
    }
    if (!selected) {
        if (reachedEnd) {
            reachedEndOfStream = true;
            lastDecodedSeconds = playableSeconds;
        }
        return;
    }

    Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
    const HRESULT contiguousResult = selected->ConvertToContiguousBuffer(&buffer);
    if (FAILED(contiguousResult)) {
        storeError(videoHresultMessage(
            "Preparing decoded session video frame", contiguousResult));
        return;
    }
    BYTE* bytes = nullptr;
    DWORD length = 0;
    const HRESULT lockResult = buffer->Lock(&bytes, nullptr, &length);
    if (FAILED(lockResult)) {
        storeError(videoHresultMessage("Locking decoded session video frame", lockResult));
        return;
    }
    if (!bytes) {
        buffer->Unlock();
        storeError("Decoded session video frame returned an empty buffer.");
        return;
    }
    const std::size_t expected = static_cast<std::size_t>(decodedStride) * height;
    if (static_cast<std::size_t>(length) < expected) {
        buffer->Unlock();
        storeError("Decoded video frame is smaller than its declared dimensions.");
        return;
    }
    auto decoded = std::make_shared<VideoFrame>();
    decoded->width = width;
    decoded->height = height;
    decoded->stride = decodedStride;
    decoded->sequence = ++frameSequence;
    decoded->timestamp100ns = selectedTimestamp;
    decoded->topDown = decodedTopDown;
    decoded->bgra.resize(expected);
    std::memcpy(decoded->bgra.data(), bytes, expected);
    buffer->Unlock();
    std::lock_guard<std::mutex> frameLock(frameMutex);
    frame = std::move(decoded);
    reachedEndOfStream = reachedEnd;
    lastDecodedSeconds = reachedEnd
        ? playableSeconds
        : static_cast<double>(selectedTimestamp) / 10'000'000.0;
}

void VideoPlaybackService::Implementation::runWorker() noexcept
{
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
        storeError(videoHresultMessage("Initializing video playback COM worker", comResult));
        playbackState.store(VideoPlaybackState::Failed, std::memory_order_release);
        return;
    }
    std::string openError;
    if (!openReader(requestedPath, openError)) {
        storeError(openError);
        playbackState.store(VideoPlaybackState::Failed, std::memory_order_release);
        if (SUCCEEDED(comResult)) {
            CoUninitialize();
        }
        return;
    }
    if (stopRequested.load(std::memory_order_acquire)) {
        if (SUCCEEDED(comResult)) CoUninitialize();
        return;
    }
    playbackState.store(VideoPlaybackState::Ready, std::memory_order_release);
    std::uint64_t handledSequence = 0;
    while (!stopRequested.load(std::memory_order_acquire)) {
        double seconds = 0.0;
        {
            std::unique_lock<std::mutex> lock(requestMutex);
            requestCondition.wait(lock, [this, &handledSequence]() {
                return stopRequested.load(std::memory_order_acquire) || requestSequence != handledSequence;
            });
            if (stopRequested.load(std::memory_order_acquire)) break;
            seconds = requestedSeconds;
            handledSequence = requestSequence;
        }
        decodeAt(seconds);
    }
    if (SUCCEEDED(comResult)) {
        CoUninitialize();
    }
}

} // namespace ovtr::win32
