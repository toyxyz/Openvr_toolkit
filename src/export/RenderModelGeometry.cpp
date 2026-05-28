#include "export/RenderModelGeometry.h"

#ifdef OVTR_HAS_OPENVR_SDK
#include "vr/OpenVRRenderModelResources.h"

#include <openvr.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <thread>

namespace ovtr {

RenderModelPositionBounds renderModelPositionBounds(const RenderModelGeometry& geometry)
{
    RenderModelPositionBounds bounds;
    if (geometry.vertices.empty()) {
        return bounds;
    }

    bounds.min = {
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max(),
    };
    bounds.max = {
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest(),
    };

    for (const RenderModelVertex& vertex : geometry.vertices) {
        for (int axis = 0; axis < 3; ++axis) {
            const double position = static_cast<double>(vertex.position[axis]);
            bounds.min[axis] = std::min(bounds.min[axis], position);
            bounds.max[axis] = std::max(bounds.max[axis], position);
        }
    }

    bounds.valid = true;
    return bounds;
}

RenderModelGeometry makeBoxRenderModelGeometry(const float edgeMeters)
{
    const float half = edgeMeters > 0.0f ? edgeMeters * 0.5f : 0.05f;
    const std::array<std::array<float, 3>, 8> corners{{
        {{-half, -half, -half}}, {{ half, -half, -half}}, {{ half,  half, -half}}, {{-half,  half, -half}},
        {{-half, -half,  half}}, {{ half, -half,  half}}, {{ half,  half,  half}}, {{-half,  half,  half}},
    }};
    const std::array<std::array<int, 4>, 6> faces{{
        {{4, 5, 6, 7}}, {{1, 0, 3, 2}}, {{3, 7, 6, 2}},
        {{0, 1, 5, 4}}, {{1, 2, 6, 5}}, {{0, 4, 7, 3}},
    }};
    const std::array<std::array<float, 3>, 6> normals{{
        {{0.0f, 0.0f, 1.0f}}, {{0.0f, 0.0f, -1.0f}}, {{0.0f, 1.0f, 0.0f}},
        {{0.0f, -1.0f, 0.0f}}, {{1.0f, 0.0f, 0.0f}}, {{-1.0f, 0.0f, 0.0f}},
    }};

    RenderModelGeometry geometry;
    geometry.available = true;
    geometry.vertices.reserve(24);
    geometry.indices.reserve(36);
    for (std::size_t faceIndex = 0; faceIndex < faces.size(); ++faceIndex) {
        const std::uint32_t base = static_cast<std::uint32_t>(geometry.vertices.size());
        for (const int cornerIndex : faces[faceIndex]) {
            RenderModelVertex vertex;
            vertex.position = corners[static_cast<std::size_t>(cornerIndex)];
            vertex.normal = normals[faceIndex];
            geometry.vertices.push_back(vertex);
        }
        geometry.indices.insert(geometry.indices.end(), {base, base + 1u, base + 2u});
        geometry.indices.insert(geometry.indices.end(), {base, base + 2u, base + 3u});
    }
    return geometry;
}

RenderModelGeometry loadSteamVRRenderModelGeometry(const std::string& renderModelName)
{
    RenderModelGeometry geometry;
    if (renderModelName.empty()) {
        return geometry;
    }

#ifdef OVTR_HAS_OPENVR_SDK
    vr::IVRRenderModels* renderModels = vr::VRRenderModels();
    if (renderModels == nullptr) {
        return geometry;
    }

    OpenVRRenderModelHandle openvrModel(renderModels);
    vr::EVRRenderModelError error = vr::VRRenderModelError_Loading;
    for (int attempt = 0; attempt < 50; ++attempt) {
        error = renderModels->LoadRenderModel_Async(renderModelName.c_str(), openvrModel.output());
        if (error != vr::VRRenderModelError_Loading) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (error != vr::VRRenderModelError_None || openvrModel.get() == nullptr) {
        return geometry;
    }

    const vr::RenderModel_t* model = openvrModel.get();
    geometry.vertices.reserve(model->unVertexCount);
    for (std::uint32_t i = 0; i < model->unVertexCount; ++i) {
        const vr::RenderModel_Vertex_t& source = model->rVertexData[i];
        RenderModelVertex vertex;
        vertex.position = {
            source.vPosition.v[0],
            source.vPosition.v[1],
            source.vPosition.v[2],
        };
        vertex.normal = {
            source.vNormal.v[0],
            source.vNormal.v[1],
            source.vNormal.v[2],
        };
        vertex.texCoord = {
            source.rfTextureCoord[0],
            source.rfTextureCoord[1],
        };
        geometry.vertices.push_back(vertex);
    }

    const std::uint32_t indexCount = model->unTriangleCount * 3;
    geometry.indices.assign(model->rIndexData, model->rIndexData + indexCount);
    geometry.available = !geometry.vertices.empty() && !geometry.indices.empty();
#else
    (void)renderModelName;
#endif

    return geometry;
}

} // namespace ovtr
