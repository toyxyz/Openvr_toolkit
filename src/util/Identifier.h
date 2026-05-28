#pragma once

#include "data/SessionTypes.h"

#include <string>

namespace ovtr {

std::string deviceClassPrefix(DeviceClass deviceClass);
std::string sanitizeIdentifier(const std::string& input);
std::string makeDeviceSafeName(const DeviceDescriptor& device);

} // namespace ovtr
