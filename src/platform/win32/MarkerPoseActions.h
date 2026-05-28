#pragma once

#include "platform/win32/AppMarkerState.h"
#include "platform/win32/AppOriginState.h"
#include "vr/IVRProvider.h"

#include <cstdint>
#include <string>

namespace ovtr::win32 {

bool addMarkerFromDevicePose(
    AppMarkerState& markerState,
    const AppOriginState& originState,
    const ovtr::PosePollResult& poses,
    std::uint32_t runtimeIndex,
    float markerSizeMeters,
    std::string& statusMessage
);

} // namespace ovtr::win32
