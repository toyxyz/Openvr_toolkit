#pragma once

#include "platform/win32/VideoFrame.h"

#include <filesystem>
#include <memory>
#include <string>

namespace ovtr::win32 {

enum class VideoPlaybackState {
    Closed,
    Opening,
    Ready,
    Failed
};

class VideoPlaybackService {
public:
    VideoPlaybackService();
    ~VideoPlaybackService();
    VideoPlaybackService(const VideoPlaybackService&) = delete;
    VideoPlaybackService& operator=(const VideoPlaybackService&) = delete;

    bool open(const std::filesystem::path& path, std::string& error);
    void close() noexcept;
    void requestFrame(double timeSeconds) noexcept;
    VideoFrameSnapshot latestFrame() const;
    std::string runtimeError() const;
    bool opened() const noexcept;
    VideoPlaybackState state() const noexcept;

private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};

} // namespace ovtr::win32
