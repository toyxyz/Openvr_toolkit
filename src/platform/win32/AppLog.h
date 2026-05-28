#pragma once

#include <string>

namespace ovtr::win32 {

struct AppDebugUiState;

void appendDebugLog(AppDebugUiState& state, const std::wstring& message);
void appendDebugLog(AppDebugUiState& state, const std::string& message);

} // namespace ovtr::win32
