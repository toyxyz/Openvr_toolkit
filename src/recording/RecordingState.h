#pragma once

namespace ovtr {

enum class RecorderState {
    Idle,
    Starting,
    Recording,
    Paused,
    Stopping,
    Finalizing,
    Error,
};

} // namespace ovtr
