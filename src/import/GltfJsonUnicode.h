#pragma once

#include <cstdint>
#include <string>

namespace ovtr::detail {

int jsonHexValue(char ch);
void appendUtf8CodePoint(std::string& output, std::uint32_t codePoint);

} // namespace ovtr::detail
