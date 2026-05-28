#include "TestCases.h"
#include "TestSupport.h"
#include "Win32ConfigTestSupport.h"

#include "platform/win32/ConfigStore.h"

#include <filesystem>
#include <string>

namespace ovtr::test {

void testWin32ConfigStoreBasics()
{
    require(ovtr::win32::trimAscii("  alpha beta \t") == "alpha beta", "trim ascii");
    require(ovtr::win32::lowerAscii("AbC-123") == "abc-123", "lower ascii");

    require(
        ovtr::win32::configFilePath("sample.cfg").filename() == "sample.cfg",
        "config file path uses requested file name"
    );
    require(
        ovtr::win32::configDirectoryPath().filename() == "config",
        "config directory path uses config folder"
    );
    require(
        ovtr::win32::defaultExportDirectoryPath() == std::filesystem::current_path() / "exports",
        "default export directory path"
    );
    require(
        ovtr::win32::normalizedExportDirectoryPath({}) == ovtr::win32::defaultExportDirectoryPath().lexically_normal(),
        "empty export directory normalizes to default"
    );
    require(
        ovtr::win32::normalizedExportDirectoryPath("relative exports") ==
            (std::filesystem::current_path() / "relative exports").lexically_normal(),
        "relative export directory normalizes against current path"
    );
    require(
        ovtr::win32::readableConfigPath("unlikely_missing_config_file_for_test.cfg") ==
            ovtr::win32::configFilePath("unlikely_missing_config_file_for_test.cfg"),
        "missing readable config falls back to preferred config path"
    );

    bool boolValue = false;
    require(ovtr::win32::parseBoolConfigValue(" yes ", boolValue) && boolValue, "parse yes");
    require(ovtr::win32::parseBoolConfigValue("No", boolValue) && !boolValue, "parse no");
    boolValue = true;
    require(!ovtr::win32::parseBoolConfigValue("maybe", boolValue) && boolValue, "reject invalid bool");

    float floatValue = 0.0f;
    require(
        ovtr::win32::parseFloatConfigValue(" 12.5 ", floatValue) &&
            win32ConfigNearlyEqual(floatValue, 12.5f),
        "parse float"
    );
    int intValue = 0;
    require(ovtr::win32::parseIntConfigValue(" 42 ", intValue) && intValue == 42, "parse int");

    ovtr::win32::ExportFormat format = ovtr::win32::ExportFormat::Fbx;
    require(
        ovtr::win32::parseExportFormatConfigValue(" gltf ", format) &&
            format == ovtr::win32::ExportFormat::Glb,
        "parse gltf as glb"
    );
    require(
        !ovtr::win32::parseExportFormatConfigValue("obj", format) &&
            format == ovtr::win32::ExportFormat::Glb,
        "reject invalid export format"
    );
    require(
        std::string(ovtr::win32::exportFormatConfigValue(ovtr::win32::ExportFormat::Fbx)) == "fbx",
        "fbx config value"
    );
    require(
        std::string(ovtr::win32::exportFormatConfigValue(ovtr::win32::ExportFormat::Glb)) == "glb",
        "glb config value"
    );
}

} // namespace ovtr::test
