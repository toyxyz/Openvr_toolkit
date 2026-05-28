#pragma once

#include <string>

namespace ovtr::win32 {

std::wstring widen(const std::string& value);
std::string narrow(const std::wstring& value);
std::wstring trimWide(std::wstring value);

} // namespace ovtr::win32
