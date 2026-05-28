#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ovtr {

std::string escapeFbxString(const std::string& input);
std::string joinedDoubles(const std::vector<double>& values);
std::string joinedInt64(const std::vector<std::int64_t>& values);
std::string joinedInt32(const std::vector<std::int32_t>& values);

} // namespace ovtr
