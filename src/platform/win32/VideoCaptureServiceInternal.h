#pragma once

#include "platform/win32/VideoFrame.h"

#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <filesystem>
#include <string>
#include <thread>

namespace ovtr::win32 {

struct VideoCaptureService::Implementation {
    HRESULT startupResult = E_FAIL;
    Microsoft::WRL::ComPtr<IMFMediaSource> mediaSource;
    Microsoft::WRL::ComPtr<IMFSourceReader> reader;
    Microsoft::WRL::ComPtr<IMFSinkWriter> writer;
    DWORD writerStream = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t frameRateNumerator = 30;
    std::uint32_t frameRateDenominator = 1;
    std::int32_t captureStride = 0;
    std::atomic<bool> stopRequested{false};
    std::atomic<bool> previewActive{false};
    std::atomic<VideoPreviewState> previewState{VideoPreviewState::Stopped};
    std::atomic<bool> recordingActive{false};
    std::atomic<bool> recordingRequested{false};
    std::atomic<bool> encoderExited{true};
    std::atomic<std::uint64_t> encoderDroppedFrames{0};
    std::atomic<VideoRecordingPreparationState> preparationState{
        VideoRecordingPreparationState::Idle};
    std::atomic<VideoFinalizationState> finalizationState{VideoFinalizationState::Idle};
    mutable std::mutex frameMutex;
    mutable std::mutex sourceMutex;
    mutable std::mutex encoderMutex;
    std::condition_variable encoderCondition;
    std::condition_variable encoderFinishedCondition;
    mutable std::mutex errorMutex;
    VideoFrameSnapshot frame;
    std::string error;
    std::thread worker;
    std::thread encoderWorker;
    std::wstring requestedCameraLink;
    std::uint64_t frameSequence = 0;
    std::filesystem::path requestedRecordingPath;
    std::filesystem::path activeRecordingFinalPath;
    std::filesystem::path activeRecordingPartialPath;
    LONGLONG recordingEpochTimestamp100ns = -1;
    LONGLONG recordingLastSampleTime100ns = -1;
    std::deque<VideoFrameSnapshot> encoderFrames;
    bool encoderStopRequested = false;
    bool encoderStartRequested = false;
    bool encoderFinishRequested = false;
    std::uint64_t encoderFinishSequence = 0;
    std::string encoderOperationError;

    bool openCamera(const std::wstring& symbolicLink, std::string& outError);
    bool createWriter(const std::filesystem::path& path, std::string& outError);
    bool finishWriter(std::string& outError) noexcept;
    void enqueueRecordingFrame(VideoFrameSnapshot snapshot);
    void runEncoder() noexcept;
    void runWorker() noexcept;
    void storeError(std::string message);
};

std::string videoHresultMessage(const char* operation, HRESULT result);

} // namespace ovtr::win32
