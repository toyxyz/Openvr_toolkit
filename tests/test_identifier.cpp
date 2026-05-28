#include "TestCases.h"
#include "TestSupport.h"

#include "export/FbxAsciiExporter.h"
#include "util/Identifier.h"

#include <string>

namespace ovtr::test {

void testIdentifierUtilities()
{
    ovtr::DeviceDescriptor device;
    device.runtimeIndex = 7;
    device.deviceClass = ovtr::DeviceClass::GenericTracker;
    device.serial = "LHR-ABC 123/!";
    require(ovtr::makeDeviceSafeName(device) == ovtr::makeFbxSafeName(device), "safe-name wrappers should match");
    require(ovtr::sanitizeIdentifier("  Hip Tracker!!  ") == "Hip_Tracker", "identifier sanitizer mismatch");

    const std::string utf8Name =
        std::string("\xED\x97\x88") + "\xEB\xA6\xAC " + "\xED\x8A\xB8" + "\xEB\x9E\x98" + "\xEC\xBB\xA4";
    const std::string utf8SafeName =
        std::string("\xED\x97\x88") + "\xEB\xA6\xAC_" + "\xED\x8A\xB8" + "\xEB\x9E\x98" + "\xEC\xBB\xA4";
    require(ovtr::sanitizeIdentifier(utf8Name) == utf8SafeName, "identifier sanitizer should keep UTF-8 bytes");
}

void testFbxSafeName()
{
    ovtr::DeviceDescriptor device;
    device.runtimeIndex = 7;
    device.deviceClass = ovtr::DeviceClass::GenericTracker;
    device.serial = "LHR-ABC 123/!";
    require(ovtr::makeFbxSafeName(device) == "Tracker_LHR_ABC_123", "FBX safe name mismatch");

    device.displayName = "Waist Tracker";
    require(ovtr::makeFbxSafeName(device) == "Waist_Tracker", "FBX custom display name mismatch");

    const std::string utf8Name =
        std::string("\xED\x97\x88") + "\xEB\xA6\xAC " + "\xED\x8A\xB8" + "\xEB\x9E\x98" + "\xEC\xBB\xA4";
    const std::string utf8SafeName =
        std::string("\xED\x97\x88") + "\xEB\xA6\xAC_" + "\xED\x8A\xB8" + "\xEB\x9E\x98" + "\xEC\xBB\xA4";
    device.displayName = utf8Name;
    require(ovtr::makeFbxSafeName(device) == utf8SafeName, "FBX UTF-8 display name mismatch");

    device.displayName.clear();
    device.serial.clear();
    require(ovtr::makeFbxSafeName(device) == "Tracker_Device_7", "FBX fallback safe name mismatch");
}

} // namespace ovtr::test
