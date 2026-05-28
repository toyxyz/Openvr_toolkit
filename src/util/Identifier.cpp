#include "util/Identifier.h"

#include <algorithm>
#include <sstream>

namespace ovtr {

std::string deviceClassPrefix(const DeviceClass deviceClass)
{
    switch (deviceClass) {
    case DeviceClass::Hmd:
        return "HMD";
    case DeviceClass::Controller:
        return "Controller";
    case DeviceClass::GenericTracker:
        return "Tracker";
    case DeviceClass::TrackingReference:
        return "TrackingReference";
    case DeviceClass::Invalid:
        return "Invalid";
    case DeviceClass::Other:
    default:
        return "Device";
    }
}

std::string sanitizeIdentifier(const std::string& input)
{
    std::string output;
    output.reserve(input.size());
    for (const char ch : input) {
        const unsigned char value = static_cast<unsigned char>(ch);
        if (value >= 0x80 ||
            (value >= 'A' && value <= 'Z') ||
            (value >= 'a' && value <= 'z') ||
            (value >= '0' && value <= '9')) {
            output.push_back(ch);
        } else {
            output.push_back('_');
        }
    }

    output.erase(
        std::unique(output.begin(), output.end(), [](const char left, const char right) {
            return left == '_' && right == '_';
        }),
        output.end()
    );

    while (!output.empty() && output.front() == '_') {
        output.erase(output.begin());
    }
    while (!output.empty() && output.back() == '_') {
        output.pop_back();
    }
    return output.empty() ? "Device" : output;
}

std::string makeDeviceSafeName(const DeviceDescriptor& device)
{
    if (!device.displayName.empty()) {
        return sanitizeIdentifier(device.displayName);
    }

    std::string source = device.serial;
    if (source.empty()) {
        std::ostringstream stream;
        stream << "Device_" << device.runtimeIndex;
        source = stream.str();
    }

    return sanitizeIdentifier(deviceClassPrefix(device.deviceClass) + "_" + source);
}

} // namespace ovtr
