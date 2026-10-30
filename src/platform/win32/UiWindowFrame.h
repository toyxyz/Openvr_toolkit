#pragma once

#include "platform/win32/UiTheme.h"

namespace ovtr::win32 {

inline void applyDarkWindowFrame(HWND hwnd)
{
    // Optional OS decoration: older Windows versions can reject dark title bars.
    HMODULE library = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!library) {
        OutputDebugStringW(L"UI theme: Windows title-bar styling library is unavailable.\n");
        return;
    }
    using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
    const auto setAttribute = reinterpret_cast<SetAttribute>(GetProcAddress(library, "DwmSetWindowAttribute"));
    if (setAttribute) {
        const BOOL dark = TRUE;
        if (FAILED(setAttribute(hwnd, 20, &dark, sizeof(dark)))) {
            OutputDebugStringW(L"UI theme: dark title bars are unsupported by this Windows version.\n");
        }
        // Windows 11 caption/text attributes; unsupported attributes are diagnostic only.
        const COLORREF caption = ui::toolbar;
        const COLORREF text = ui::text;
        const HRESULT captionResult = setAttribute(hwnd, 35, &caption, sizeof(caption));
        const HRESULT textResult = setAttribute(hwnd, 36, &text, sizeof(text));
        if (FAILED(captionResult) || FAILED(textResult)) {
            OutputDebugStringW(L"UI theme: custom title-bar colors are unsupported by this Windows version.\n");
        }
    } else {
        OutputDebugStringW(L"UI theme: DwmSetWindowAttribute is unavailable.\n");
    }
    FreeLibrary(library);
}

} // namespace ovtr::win32
