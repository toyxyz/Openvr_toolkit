#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <string>

namespace ovtr::win32 {

struct PopupMenuItem {
    UINT commandId = 0;
    std::wstring label;
};

void appendPopupMenuItem(HMENU menu, PopupMenuItem& item);
bool measurePopupMenuItem(HWND hwnd, MEASUREITEMSTRUCT& measure);
bool drawPopupMenuItem(const DRAWITEMSTRUCT& draw);

} // namespace ovtr::win32
