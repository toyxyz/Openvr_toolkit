#pragma once

#include "platform/win32/Win32GlTextureResource.h"
#include "platform/win32/ViewportGpuMesh.h"
#include "platform/win32/ViewportTriangleDisplayListCache.h"

#include <array>
#include <cstdint>
#include <vector>

namespace ovtr::win32 {

struct RenderModelVertex {
    std::array<float, 3> position{0.0f, 0.0f, 0.0f};
    std::array<float, 3> normal{0.0f, 1.0f, 0.0f};
    std::array<float, 2> texCoord{0.0f, 0.0f};
};

struct RenderModelMesh {
    enum class LoadState {
        Pending,
        Ready,
        Failed,
    };

    LoadState state = LoadState::Pending;
    std::vector<RenderModelVertex> vertices;
    std::vector<std::uint16_t> indices;
    ViewportGpuMesh surfaceGpuMesh;
    ViewportTriangleDisplayListCache surfaceDisplayList;
    int diffuseTextureId = -1;
    UniqueGlTexture texture;
    bool textureAvailable = false;
    bool textureLoadFailed = false;
};

} // namespace ovtr::win32
