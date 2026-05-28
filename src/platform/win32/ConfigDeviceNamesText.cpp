#include "platform/win32/ConfigStore.h"

#include <iomanip>
#include <sstream>
#include <utility>

namespace ovtr::win32 {

DeviceNameConfigParseResult parseDeviceNameConfig(std::istream& input)
{
    DeviceNameConfigParseResult result;

    std::string line;
    while (std::getline(input, line)) {
        line = trimAscii(line);
        if (line.empty() || line.front() == '#') {
            continue;
        }

        std::istringstream stream(line);
        DeviceNameConfigEntry entry;
        if (!(stream >> std::quoted(entry.deviceClass) >> std::quoted(entry.serial) >> std::quoted(entry.customName))) {
            ++result.invalidLineCount;
            continue;
        }

        entry.customName = trimAscii(entry.customName);
        if (!entry.customName.empty()) {
            result.entries.push_back(std::move(entry));
        }
    }

    return result;
}

std::string serializeDeviceNameConfig(const std::vector<DeviceNameConfigEntry>& entries)
{
    std::ostringstream output;
    output << "# device_class serial custom_name\n";
    for (const DeviceNameConfigEntry& entry : entries) {
        if (entry.customName.empty()) {
            continue;
        }

        output << std::quoted(entry.deviceClass) << " "
               << std::quoted(entry.serial) << " "
               << std::quoted(entry.customName) << "\n";
    }
    return output.str();
}

} // namespace ovtr::win32
