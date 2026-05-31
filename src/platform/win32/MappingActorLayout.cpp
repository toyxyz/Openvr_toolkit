#include "platform/win32/MappingActorLayout.h"

namespace ovtr::win32 {
namespace {

constexpr int kActorRowHeight = 28;

} // namespace

std::vector<MappingActorRowLayout> mappingActorRowLayouts(
    const MappingPanelControlsLayout& controls,
    const std::size_t actorCount,
    const int scrollOffset
)
{
    std::vector<MappingActorRowLayout> rows;
    if (!controls.valid || controls.actorListRect.bottom <= controls.actorListRect.top || actorCount == 0) {
        return rows;
    }

    const int visibleRows = visibleMappingActorRowCount(controls);
    const int first = clampMappingActorScrollOffset(scrollOffset, actorCount, visibleRows);
    const std::size_t remaining = actorCount - static_cast<std::size_t>(first);
    const std::size_t count = remaining < static_cast<std::size_t>(visibleRows)
        ? remaining
        : static_cast<std::size_t>(visibleRows);
    rows.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const int top = controls.actorListRect.top + static_cast<int>(index) * kActorRowHeight;
        MappingActorRowLayout row;
        row.rowRect = {controls.actorListRect.left, top, controls.actorListRect.right, top + kActorRowHeight};
        row.nameRect = {row.rowRect.left + 8, row.rowRect.top, row.rowRect.right - 16, row.rowRect.bottom};
        row.actorIndex = static_cast<std::size_t>(first) + index;
        row.valid = true;
        rows.push_back(row);
    }
    return rows;
}

MappingActorRowLayout mappingActorRowAtPoint(
    const MappingPanelControlsLayout& controls,
    const std::size_t actorCount,
    const int scrollOffset,
    const POINT point
)
{
    for (const MappingActorRowLayout& row : mappingActorRowLayouts(controls, actorCount, scrollOffset)) {
        if (PtInRect(&row.rowRect, point)) {
            return row;
        }
    }
    return {};
}

int visibleMappingActorRowCount(const MappingPanelControlsLayout& controls) noexcept
{
    if (!controls.valid) {
        return 0;
    }
    const int rows = (controls.actorListRect.bottom - controls.actorListRect.top) / kActorRowHeight;
    return rows > 0 ? rows : 0;
}

int maxMappingActorScrollOffset(const std::size_t actorCount, const int visibleRowCount) noexcept
{
    if (visibleRowCount <= 0 || actorCount <= static_cast<std::size_t>(visibleRowCount)) {
        return 0;
    }
    return static_cast<int>(actorCount) - visibleRowCount;
}

int clampMappingActorScrollOffset(const int scrollOffset, const std::size_t actorCount, const int visibleRowCount) noexcept
{
    const int maxOffset = maxMappingActorScrollOffset(actorCount, visibleRowCount);
    if (scrollOffset < 0) {
        return 0;
    }
    return scrollOffset > maxOffset ? maxOffset : scrollOffset;
}

} // namespace ovtr::win32
