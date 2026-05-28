#include "recording/RecordingController.h"

#include <filesystem>

namespace ovtr {

bool RecordingController::start(const RecordingStartOptions& options)
{
    reset();
    state_ = RecorderState::Starting;
    lastError_.clear();
    sessionFolder_ = options.sessionFolder;
    session_ = options.session;

    if (sessionFolder_.empty()) {
        setError("session folder is empty");
        return false;
    }

    std::error_code error;
    std::filesystem::create_directories(sessionFolder_, error);
    if (error) {
        setError("failed to create session folder: " + error.message());
        return false;
    }

    if (session_.framesPath.empty()) {
        session_.framesPath = sessionFolder_ / "frames.bin";
    }
    if (session_.frameIndexPath.empty()) {
        session_.frameIndexPath = sessionFolder_ / "frame_index.bin";
    }

    if (!writer_.open(session_.framesPath, session_.frameIndexPath)) {
        setError(writer_.lastError());
        return false;
    }

    if (!writeManifest(false, 0.0, 0)) {
        return false;
    }

    frameCount_ = 0;
    state_ = RecorderState::Recording;
    return true;
}

bool RecordingController::appendFrame(const FrameSample& frame)
{
    if (state_ != RecorderState::Recording) {
        setError("recorder is not recording");
        return false;
    }

    if (!writer_.appendFrame(frame)) {
        setError(writer_.lastError());
        return false;
    }

    frameCount_ = writer_.frameCount();
    return true;
}

bool RecordingController::pause()
{
    if (state_ != RecorderState::Recording) {
        setError("recorder is not recording");
        return false;
    }

    state_ = RecorderState::Paused;
    return true;
}

bool RecordingController::resume()
{
    if (state_ != RecorderState::Paused) {
        setError("recorder is not paused");
        return false;
    }

    state_ = RecorderState::Recording;
    return true;
}

bool RecordingController::stop(const double durationSeconds, const std::uint64_t droppedFrames)
{
    if (state_ != RecorderState::Recording && state_ != RecorderState::Paused) {
        setError("recorder is not active");
        return false;
    }

    state_ = RecorderState::Stopping;
    writer_.close();

    state_ = RecorderState::Finalizing;
    if (!writeManifest(true, durationSeconds, droppedFrames)) {
        return false;
    }

    state_ = RecorderState::Idle;
    return true;
}

} // namespace ovtr
