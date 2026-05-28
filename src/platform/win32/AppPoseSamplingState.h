#pragma once

#include "vr/IVRProvider.h"

#include <atomic>
#include <mutex>
#include <thread>

namespace ovtr::win32 {

struct AppPoseSamplingState {
    std::thread poseSamplingThread;
    std::atomic_bool poseSamplingStopRequested{false};
    mutable std::mutex providerMutex;
    mutable std::mutex poseMutex;
    ovtr::PosePollResult latestPoseSnapshot;
};

} // namespace ovtr::win32
