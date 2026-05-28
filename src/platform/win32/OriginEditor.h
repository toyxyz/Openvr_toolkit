#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;
struct OriginStepperButton;

void closeOriginEditor(HWND hwnd, AppWindowState& state);
void applyOriginStepperButton(HWND hwnd, AppWindowState& state, const OriginStepperButton& button);
void showOriginEditor(HWND hwnd, AppWindowState& state);

} // namespace ovtr::win32
