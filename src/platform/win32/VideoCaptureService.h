#pragma once

#include "platform/win32/VideoFrame.h"

#include <filesystem>
#include <cstdint>
#include <memory>
#include <string>

namespace ovtr::win32 {

enum class VideoFinalizationState {
    Idle,
    Pending,
    Succeeded,
    Failed
};

enum class VideoRecordingPreparationState {
    Idle,
    Pending,
    Ready,
    Failed
};

enum class VideoPreviewState {
    Stopped,
    Opening,
    Ready,
    Failed
};

class VideoCaptureService {
public:
    VideoCaptureService();
    ~VideoCaptureService();
    VideoCaptureService(const VideoCaptureService&) = delete;
    VideoCaptureService& operator=(const VideoCaptureService&) = delete;

    bool startPreview(const std::wstring& symbolicLink, std::string& error);
    void stopPreview() noexcept;
    bool beginRecording(const std::filesystem::path& outputPath, std::string& error);
    bool activatePreparedRecording(std::string& error) noexcept;
    bool requestEndRecording(std::string& error) noexcept;
    bool endRecording(std::string& error) noexcept;
    VideoFrameSnapshot latestFrame() const;
    bool previewing() const noexcept;
    VideoPreviewState previewState() const noexcept;
    bool recording() const noexcept;
    std::uint64_t droppedRecordingFrames() const noexcept;
    VideoRecordingPreparationState preparationState() const noexcept;
    std::string preparationError() const;
    VideoFinalizationState finalizationState() const noexcept;
    std::string finalizationError() const;
    std::string runtimeError() const;

private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};

} // namespace ovtr::win32
