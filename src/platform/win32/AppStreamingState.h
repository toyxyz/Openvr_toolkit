#pragma once

#include "platform/win32/ConfigTypes.h"
#include "platform/win32/RealtimePoseSmoothing.h"

#include <mutex>

namespace ovtr::win32 {

struct AppStreamingState {
    mutable std::mutex realtimeSmoothingMutex;
    bool realtimeSmoothingEnabled = false;
    RealtimeSmoothingPreset realtimeSmoothingPreset = RealtimeSmoothingPreset::Normal;
    RealtimePoseSmoother realtimePoseSmoother;
};

} // namespace ovtr::win32
