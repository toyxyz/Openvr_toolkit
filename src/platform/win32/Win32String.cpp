#include "platform/win32/Win32String.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cwctype>

namespace ovtr::win32 {

std::wstring widen(const std::string& value)
{
    if (value.empty()) {
        return {};
    }

    const int required = MultiByteToWideChar(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0
    );

    if (required <= 0) {
        return {};
    }

    std::wstring output(static_cast<std::size_t>(required), L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        output.data(),
        required
    );
    return output;
}

std::string narrow(const std::wstring& value)
{
    if (value.empty()) {
        return {};
    }

    const int required = WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (required <= 0) {
        return {};
    }

    std::string output(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        output.data(),
        required,
        nullptr,
        nullptr
    );
    return output;
}

std::wstring trimWide(std::wstring value)
{
    auto first = value.begin();
    while (first != value.end() && std::iswspace(*first)) {
        ++first;
    }
    auto last = value.end();
    while (last != first && std::iswspace(*(last - 1))) {
        --last;
    }
    return std::wstring(first, last);
}

} // namespace ovtr::win32
