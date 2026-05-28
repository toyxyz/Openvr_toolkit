#include "TestCases.h"
#include "TestSupport.h"
#include "Win32ConfigTestSupport.h"

#include "platform/win32/ConfigStore.h"

#include <sstream>
#include <string>

namespace ovtr::test {

void testWin32ConfigStore()
{
    std::istringstream deviceNameInput(
        "# header\n"
        "\"Controller\" \"serial 1\" \" Left Hand \"\n"
        "invalid-line\n"
        "\"Tracker\" \"serial 2\" \"\"\n"
    );
    const ovtr::win32::DeviceNameConfigParseResult deviceNames =
        ovtr::win32::parseDeviceNameConfig(deviceNameInput);
    require(deviceNames.entries.size() == 1, "parse non-empty device custom name");
    require(deviceNames.entries.front().deviceClass == "Controller", "parse device class");
    require(deviceNames.entries.front().serial == "serial 1", "parse device serial");
    require(deviceNames.entries.front().customName == "Left Hand", "trim parsed custom name");
    require(deviceNames.invalidLineCount == 1, "count invalid device-name lines");

    const std::string serializedDeviceNames = ovtr::win32::serializeDeviceNameConfig({
        {"Controller", "serial 1", "Left Hand"},
        {"Tracker", "serial 2", ""},
    });
    require(
        serializedDeviceNames.find("# device_class serial custom_name") == 0,
        "serialize device-name header"
    );
    require(
        serializedDeviceNames.find("\"Controller\" \"serial 1\" \"Left Hand\"") != std::string::npos,
        "serialize quoted device-name entry"
    );
    require(
        serializedDeviceNames.find("serial 2") == std::string::npos,
        "skip empty device-name entry"
    );

    std::istringstream originInput(
        "enabled=1\n"
        "x=1.5\n"
        "y=-2.5\n"
        "z=3.0\n"
        "rx=4\n"
        "ry=5\n"
        "rz=6\n"
    );
    const ovtr::win32::OriginConfigParseResult origin = ovtr::win32::parseOriginConfig(originInput);
    require(origin.status == ovtr::win32::OriginConfigParseStatus::Loaded, "parse enabled origin");
    require(win32ConfigNearlyEqual(origin.config.offset[0], 1.5f), "parse origin x");
    require(win32ConfigNearlyEqual(origin.config.rotationDegrees[2], 6.0f), "parse origin rz");

    std::istringstream disabledOriginInput("enabled=0\nx=8\ny=9\nz=10\n");
    const ovtr::win32::OriginConfigParseResult disabledOrigin =
        ovtr::win32::parseOriginConfig(disabledOriginInput);
    require(disabledOrigin.status == ovtr::win32::OriginConfigParseStatus::Disabled, "parse disabled origin");
    require(win32ConfigNearlyEqual(disabledOrigin.config.offset[0], 0.0f), "disabled origin clears offset");

    std::istringstream incompleteOriginInput("enabled=1\nx=1\nz=3\n");
    const ovtr::win32::OriginConfigParseResult incompleteOrigin =
        ovtr::win32::parseOriginConfig(incompleteOriginInput);
    require(
        incompleteOrigin.status == ovtr::win32::OriginConfigParseStatus::MissingCoordinates,
        "reject incomplete origin"
    );

    ovtr::win32::OriginConfig originConfig;
    originConfig.enabled = true;
    originConfig.offset = {1.0f, 2.0f, 3.0f};
    originConfig.rotationDegrees = {4.0f, 5.0f, 6.0f};
    const std::string serializedOrigin = ovtr::win32::serializeOriginConfig(originConfig);
    require(serializedOrigin.find("enabled=1") != std::string::npos, "serialize origin enabled");
    require(serializedOrigin.find("x=1.000000000") != std::string::npos, "serialize origin precision");
}

} // namespace ovtr::test
