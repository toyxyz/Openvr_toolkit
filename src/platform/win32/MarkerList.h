#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/AppMarkerState.h"
#include "platform/win32/LayoutTypes.h"

#include <cstdint>
#include <string>

namespace ovtr::win32 {

std::string markerNameForId(std::uint32_t markerId);
const SceneMarker* markerForId(const AppMarkerState& state, std::uint32_t markerId);
SceneMarker* markerForId(AppMarkerState& state, std::uint32_t markerId);
void selectMarker(AppMarkerState& state, std::uint32_t markerId);
void toggleMarkerSelection(AppMarkerState& state, std::uint32_t markerId);
SceneMarker& appendMarker(
    AppMarkerState& state,
    const std::array<float, 3>& position,
    const std::array<float, 4>& rotation,
    float sizeMeters
);
bool renameMarker(AppMarkerState& state, std::uint32_t markerId, const std::string& name);
bool deleteMarker(AppMarkerState& state, std::uint32_t markerId);
int maxMarkerListScrollOffset(int totalItemCount, int visibleItemCount) noexcept;
int clampMarkerListScrollOffset(int scrollOffset, int totalItemCount, int visibleItemCount) noexcept;
int markerListItemTextRight(const MarkerListLayout& layout, int totalItemCount) noexcept;
int markerListRowIndexFromPoint(
    const MarkerListLayout& layout,
    POINT point,
    int totalItemCount,
    int scrollOffset
) noexcept;
std::uint32_t markerIdFromListPoint(
    const AppMarkerState& state,
    const MarkerListLayout& layout,
    POINT point
) noexcept;

} // namespace ovtr::win32
