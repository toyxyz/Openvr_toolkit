#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ovtr::win32 {

inline constexpr std::uint32_t kNoSelectedMarkerId = 0;
inline constexpr float kDefaultMarkerBoxSizeMeters = 0.10f;

struct SceneMarker {
    std::uint32_t id = 0;
    std::string name;
    std::array<float, 3> position{0.0f, 0.0f, 0.0f};
    std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};
    float sizeMeters = kDefaultMarkerBoxSizeMeters;
};

struct AppMarkerState {
    std::vector<SceneMarker> markers;
    std::uint32_t nextMarkerId = 1;
    std::uint32_t selectedMarkerId = kNoSelectedMarkerId;
    int markerListScrollOffset = 0;
};

} // namespace ovtr::win32
