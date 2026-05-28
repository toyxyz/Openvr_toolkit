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
    std::vector<std::uint32_t> indices;
};

struct RenderModelPositionBounds {
    std::array<double, 3> min{0.0, 0.0, 0.0};
    std::array<double, 3> max{0.0, 0.0, 0.0};
    bool valid = false;
};

RenderModelPositionBounds renderModelPositionBounds(const RenderModelGeometry& geometry);
RenderModelGeometry makeBoxRenderModelGeometry(float edgeMeters);
RenderModelGeometry makeBoxRenderModelGeometry(float lengthX, float heightY, float depthZ);
RenderModelGeometry loadSteamVRRenderModelGeometry(const std::string& renderModelName);

} // namespace ovtr
