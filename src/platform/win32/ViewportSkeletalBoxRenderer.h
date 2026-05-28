#pragma once

#include "vr/IVRProvider.h"

namespace ovtr::win32 {

struct AppOriginState;
struct AppViewportState;

void drawSkeletalFingerBoxes3D(
    const ovtr::PosePollResult& poses,
    const AppOriginState& originState,
    AppViewportState& viewportState
);

} // namespace ovtr::win32
