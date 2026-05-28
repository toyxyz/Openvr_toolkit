#pragma once

#include "platform/win32/AppStateConstants.h"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace ovtr::win32 {

struct AppDeviceState {
    std::unordered_map<std::string, std::string> deviceCustomNames;
    int deviceListScrollOffset = 0;
    std::uint32_t selectedDeviceRuntimeIndex = kNoSelectedRuntimeIndex;
};

} // namespace ovtr::win32
