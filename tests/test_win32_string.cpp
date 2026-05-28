#include "TestCases.h"
#include "TestSupport.h"

#include "platform/win32/Win32String.h"

#include <string>

namespace ovtr::test {

void testWin32String()
{
    require(ovtr::win32::widen("").empty(), "empty UTF-8 widens to empty string");
    require(ovtr::win32::narrow(L"").empty(), "empty wide string narrows to empty string");

    const std::string utf8 = "Tracker \xEC\x9D\xB4\xEB\xA6\x84";
    const std::wstring wide = ovtr::win32::widen(utf8);
    require(!wide.empty(), "UTF-8 text widens");
    require(ovtr::win32::narrow(wide) == utf8, "UTF-8 text round-trips through wide string");

    require(ovtr::win32::trimWide(L"  origin values \t") == L"origin values", "wide trim removes whitespace");
}

} // namespace ovtr::test
