#pragma once

#include "platform/win32/ConfigStore.h"

#include <string>

namespace ovtr::win32::detail {

struct ConfigAssignment {
    std::string key;
    std::string value;
};

inline bool parseConfigAssignmentLine(const std::string& line, ConfigAssignment& assignment)
{
    const std::size_t separator = line.find('=');
    if (separator == std::string::npos) {
        return false;
    }

    assignment.key = trimAscii(line.substr(0, separator));
    assignment.value = trimAscii(line.substr(separator + 1));
    return true;
}

} // namespace ovtr::win32::detail
