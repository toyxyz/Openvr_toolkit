#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

inline constexpr UINT kDeviceContextMenuSetOriginId = 1001;
inline constexpr UINT kDeviceContextMenuSetNameId = 1002;
inline constexpr UINT kDeviceContextMenuAddMarkerId = 1003;
inline constexpr UINT kMarkerContextMenuRenameId = 1101;
inline constexpr UINT kMarkerContextMenuDeleteId = 1102;
inline constexpr UINT kMappingActorContextMenuResetId = 1201;
inline constexpr UINT kMappingActorContextMenuDeleteId = 1202;
inline constexpr UINT kSessionContextMenuLoadId = 1300;
inline constexpr UINT kSessionContextMenuDeleteId = 1301;

} // namespace ovtr::win32
