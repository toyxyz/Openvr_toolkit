#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

inline constexpr const wchar_t* kExportProgressDialogClassName = L"ToyxyzExportProgressDialog";

bool registerExportProgressDialogClass(HINSTANCE instance);
void showExportProgressDialog(HWND owner, AppWindowState& state);
void updateExportProgressDialog(AppWindowState& state);
void hideExportProgressDialog(AppWindowState& state);

} // namespace ovtr::win32
