#pragma once

#include "platform/win32/VideoFrame.h"

#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

namespace ovtr::win32 {

struct VideoPlaybackService::Implementation {
    HRESULT startupResult = E_FAIL;
    mutable std::mutex readerMutex;
    Microsoft::WRL::ComPtr<IMFSourceReader> reader;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t decodedStride = 0;
    bool decodedTopDown = true;
    std::atomic<bool> stopRequested{false};
    std::atomic<VideoPlaybackState> playbackState{VideoPlaybackState::Closed};
    mutable std::mutex requestMutex;
    std::condition_variable requestCondition;
    double requestedSeconds = 0.0;
    std::uint64_t requestSequence = 0;
    mutable std::mutex frameMutex;
    VideoFrameSnapshot frame;
    mutable std::mutex errorMutex;
    std::string error;
    std::thread worker;
    std::filesystem::path requestedPath;
    std::uint64_t frameSequence = 0;
    double lastDecodedSeconds = -1.0;
    double frameDurationSeconds = 1.0 / 30.0;
    double durationSeconds = 0.0;
    bool reachedEndOfStream = false;

    bool openReader(const std::filesystem::path& path, std::string& outError);
    void runWorker() noexcept;
    void decodeAt(double seconds) noexcept;
    void storeError(std::string message);
};

} // namespace ovtr::win32
