#include "platform/win32/ConfigStore.h"

namespace ovtr::win32 {

std::filesystem::path originConfigPath()
{
    return configFilePath(kOriginConfigFileName);
}

std::filesystem::path deviceNameConfigPath()
{
    return configFilePath(kDeviceNameConfigFileName);
}

std::filesystem::path viewportSettingsConfigPath()
{
    return configFilePath(kViewportSettingsConfigFileName);
}

std::filesystem::path recordSettingsConfigPath()
{
    return configFilePath(kRecordSettingsConfigFileName);
}

} // namespace ovtr::win32
