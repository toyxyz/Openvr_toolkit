#pragma once

#include <iosfwd>
#include <string>

namespace ovtr {

std::string escapeJsonString(const std::string& input);
void writeJsonString(std::ostream& output, const std::string& input);

} // namespace ovtr
