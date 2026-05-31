#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/MappingPanelLayout.h"

#include <cstddef>
#include <vector>

namespace ovtr::win32 {

struct MappingActorRowLayout {
    RECT rowRect{0, 0, 0, 0};
    RECT nameRect{0, 0, 0, 0};
    std::size_t actorIndex = 0;
    bool valid = false;
};

std::vector<MappingActorRowLayout> mappingActorRowLayouts(
    const MappingPanelControlsLayout& controls,
    std::size_t actorCount,
    int scrollOffset = 0
);
MappingActorRowLayout mappingActorRowAtPoint(
    const MappingPanelControlsLayout& controls,
    std::size_t actorCount,
    int scrollOffset,
    POINT point
);
int visibleMappingActorRowCount(const MappingPanelControlsLayout& controls) noexcept;
int maxMappingActorScrollOffset(std::size_t actorCount, int visibleRowCount) noexcept;
int clampMappingActorScrollOffset(int scrollOffset, std::size_t actorCount, int visibleRowCount) noexcept;

} // namespace ovtr::win32
