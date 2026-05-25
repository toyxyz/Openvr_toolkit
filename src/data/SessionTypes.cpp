#include "data/SessionTypes.h"

namespace ovtr {

std::string toString(const DeviceClass deviceClass)
{
    switch (deviceClass) {
    case DeviceClass::Invalid:
        return "Invalid";
    case DeviceClass::Hmd:
        return "HMD";
    case DeviceClass::Controller:
        return "Controller";
    case DeviceClass::GenericTracker:
        return "GenericTracker";
    case DeviceClass::TrackingReference:
        return "TrackingReference";
    case DeviceClass::Other:
        return "Other";
    }

    return "Invalid";
}

} // namespace ovtr

