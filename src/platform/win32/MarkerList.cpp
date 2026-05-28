#include "platform/win32/MarkerList.h"

#include "platform/win32/DeviceListLayoutMetrics.h"
#include "platform/win32/Layout.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace ovtr::win32 {

std::string markerNameForId(const std::uint32_t markerId)
{
    std::ostringstream stream;
    stream << "marker_" << markerId;
    return stream.str();
}

const SceneMarker* markerForId(const AppMarkerState& state, const std::uint32_t markerId)
{
    const auto found = std::find_if(state.markers.begin(), state.markers.end(), [markerId](const SceneMarker& marker) {
        return marker.id == markerId;
    });
    return found == state.markers.end() ? nullptr : &(*found);
}

SceneMarker* markerForId(AppMarkerState& state, const std::uint32_t markerId)
{
    auto found = std::find_if(state.markers.begin(), state.markers.end(), [markerId](const SceneMarker& marker) {
        return marker.id == markerId;
    });
    return found == state.markers.end() ? nullptr : &(*found);
}

void selectMarker(AppMarkerState& state, const std::uint32_t markerId)
{
    state.selectedMarkerId = markerForId(state, markerId) ? markerId : kNoSelectedMarkerId;
}

void toggleMarkerSelection(AppMarkerState& state, const std::uint32_t markerId)
{
    if (state.selectedMarkerId == markerId) {
        state.selectedMarkerId = kNoSelectedMarkerId;
        return;
    }
    selectMarker(state, markerId);
}

SceneMarker& appendMarker(
    AppMarkerState& state,
    const std::array<float, 3>& position,
    const std::array<float, 4>& rotation,
    const float sizeMeters
)
{
    const std::uint32_t markerId = state.nextMarkerId++;
    SceneMarker marker;
    marker.id = markerId;
    marker.name = markerNameForId(markerId);
    marker.position = position;
    marker.rotation = rotation;
    marker.sizeMeters = sizeMeters;
    state.markers.push_back(std::move(marker));
    state.selectedMarkerId = markerId;
    return state.markers.back();
}

bool renameMarker(AppMarkerState& state, const std::uint32_t markerId, const std::string& name)
{
    SceneMarker* marker = markerForId(state, markerId);
    if (!marker || name.empty()) {
        return false;
    }
    marker->name = name;
    return true;
}

bool deleteMarker(AppMarkerState& state, const std::uint32_t markerId)
{
    const auto oldSize = state.markers.size();
    state.markers.erase(
        std::remove_if(state.markers.begin(), state.markers.end(), [markerId](const SceneMarker& marker) {
            return marker.id == markerId;
        }),
        state.markers.end()
    );
    if (state.markers.size() == oldSize) {
        return false;
    }
    if (state.selectedMarkerId == markerId) {
        state.selectedMarkerId = kNoSelectedMarkerId;
    }
    return true;
}

int maxMarkerListScrollOffset(const int totalItemCount, const int visibleItemCount) noexcept
{
    return maxDeviceListScrollOffset(totalItemCount, visibleItemCount);
}

int clampMarkerListScrollOffset(
    const int scrollOffset,
    const int totalItemCount,
    const int visibleItemCount
) noexcept
{
    return clampDeviceListScrollOffset(scrollOffset, totalItemCount, visibleItemCount);
}

int markerListItemTextRight(const MarkerListLayout& layout, const int totalItemCount) noexcept
{
    if (!layout.valid) {
        return 0;
    }
    return maxMarkerListScrollOffset(totalItemCount, layout.visibleItemCount) > 0
        ? layout.contentRect.right - 14
        : layout.contentRect.right;
}

int markerListRowIndexFromPoint(
    const MarkerListLayout& layout,
    const POINT point,
    const int totalItemCount,
    const int scrollOffset
) noexcept
{
    if (!layout.valid || totalItemCount <= 0 || !PtInRect(&layout.contentRect, point)) {
        return -1;
    }
    if (point.x >= markerListItemTextRight(layout, totalItemCount)) {
        return -1;
    }
    const int visibleRowIndex = (point.y - layout.contentRect.top) / kDeviceListItemHeight;
    if (visibleRowIndex < 0 || visibleRowIndex >= layout.visibleItemCount) {
        return -1;
    }
    const int rowIndex = scrollOffset + visibleRowIndex;
    return rowIndex >= 0 && rowIndex < totalItemCount ? rowIndex : -1;
}

std::uint32_t markerIdFromListPoint(
    const AppMarkerState& state,
    const MarkerListLayout& layout,
    const POINT point
) noexcept
{
    const int rowIndex = markerListRowIndexFromPoint(
        layout,
        point,
        static_cast<int>(state.markers.size()),
        state.markerListScrollOffset
    );
    return rowIndex < 0 ? kNoSelectedMarkerId : state.markers[static_cast<std::size_t>(rowIndex)].id;
}

} // namespace ovtr::win32
