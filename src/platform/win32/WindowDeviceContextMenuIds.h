#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

inline constexpr UINT kDeviceContextMenuSetOriginId = 1001;
inline constexpr UINT kDeviceContextMenuSetNameId = 1002;

} // namespace ovtr::win32
