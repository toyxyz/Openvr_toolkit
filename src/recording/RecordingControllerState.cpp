#include "recording/RecordingController.h"

namespace ovtr {

void RecordingController::reset()
{
    writer_.close();
    state_ = RecorderState::Idle;
    session_ = {};
    sessionFolder_.clear();
    frameCount_ = 0;
    lastError_.clear();
}

RecorderState RecordingController::state() const
{
    return state_;
}

const RecordingSession& RecordingController::session() const
{
    return session_;
}

std::uint64_t RecordingController::frameCount() const
{
    return frameCount_;
}

const std::string& RecordingController::lastError() const
{
    return lastError_;
}

} // namespace ovtr
