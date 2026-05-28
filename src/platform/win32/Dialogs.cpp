#include "platform/win32/Dialogs.h"

#include "platform/win32/ConfigStore.h"
#include "platform/win32/Win32String.h"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <cwctype>
#include <iomanip>
#include <iterator>
#include <sstream>

namespace ovtr::win32 {
namespace {

bool hasOnlyTrailingWhitespace(const wchar_t* cursor) noexcept
{
    while (*cursor != L'\0') {
        if (std::iswspace(*cursor) == 0) {
            return false;
        }
        ++cursor;
    }
    return true;
}

} // namespace

std::wstring formatIntegerText(const int value)
{
    return std::to_wstring(value);
}

std::wstring formatFloatText(const float value)
{
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(3) << value;
    return stream.str();
}

void setEditText(HWND editWindow, const std::wstring& text)
{
    if (editWindow) {
        SetWindowTextW(editWindow, text.c_str());
    }
}

std::wstring readWindowText(HWND window)
{
    if (!window) {
        return {};
    }

    const int textLength = GetWindowTextLengthW(window);
    std::wstring text(static_cast<std::size_t>(textLength) + 1, L'\0');
    GetWindowTextW(window, text.data(), textLength + 1);
    text.resize(static_cast<std::size_t>(textLength));
    return text;
}

std::wstring readTrimmedWindowText(HWND window)
{
    return trimWide(readWindowText(window));
}

bool readIntegerEdit(HWND editWindow, int& value)
{
    if (!editWindow) {
        return false;
    }

    std::wstring text = readWindowText(editWindow);
    wchar_t* begin = text.data();
    wchar_t* end = nullptr;
    const long parsed = std::wcstol(begin, &end, 10);
    if (end == begin || !hasOnlyTrailingWhitespace(end)) {
        return false;
    }

    value = clampColorComponent(static_cast<int>(parsed));
    return true;
}

bool readFloatEdit(HWND editWindow, float& value)
{
    if (!editWindow) {
        return false;
    }

    std::wstring text = readWindowText(editWindow);
    wchar_t* begin = text.data();
    wchar_t* end = nullptr;
    const float parsed = std::wcstof(begin, &end);
    if (end == begin || !std::isfinite(parsed) || !hasOnlyTrailingWhitespace(end)) {
        return false;
    }

    value = std::clamp(parsed, 0.0f, 10.0f);
    return true;
}

bool readFiniteFloatEdit(HWND editWindow, float& value)
{
    if (!editWindow) {
        return false;
    }

    std::wstring text = readWindowText(editWindow);
    wchar_t* begin = text.data();
    wchar_t* end = nullptr;
    const float parsed = std::wcstof(begin, &end);
    if (end == begin || !std::isfinite(parsed) || !hasOnlyTrailingWhitespace(end)) {
        return false;
    }

    value = parsed;
    return true;
}

} // namespace ovtr::win32
