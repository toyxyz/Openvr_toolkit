#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ovtr {

struct RenderModelVertex {
    std::array<float, 3> position{0.0f, 0.0f, 0.0f};
    std::array<float, 3> normal{0.0f, 1.0f, 0.0f};
    std::array<float, 2> texCoord{0.0f, 0.0f};
};

struct RenderModelGeometry {
    bool available = false;
    std::vector<RenderModelVertex> vertices;
    std::vector<std::uint16_t> indices;
};

RenderModelGeometry loadSteamVRRenderModelGeometry(const std::string& renderModelName);

} // namespace ovtr
