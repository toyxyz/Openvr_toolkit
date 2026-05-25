#include "export/RenderModelGeometry.h"

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>
#endif

#include <chrono>
#include <thread>

namespace ovtr {

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

    vr::RenderModel_t* openvrModel = nullptr;
    vr::EVRRenderModelError error = vr::VRRenderModelError_Loading;
    for (int attempt = 0; attempt < 50; ++attempt) {
        error = renderModels->LoadRenderModel_Async(renderModelName.c_str(), &openvrModel);
        if (error != vr::VRRenderModelError_Loading) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (error != vr::VRRenderModelError_None || openvrModel == nullptr) {
        return geometry;
    }

    geometry.vertices.reserve(openvrModel->unVertexCount);
    for (std::uint32_t i = 0; i < openvrModel->unVertexCount; ++i) {
        const vr::RenderModel_Vertex_t& source = openvrModel->rVertexData[i];
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

    const std::uint32_t indexCount = openvrModel->unTriangleCount * 3;
    geometry.indices.assign(openvrModel->rIndexData, openvrModel->rIndexData + indexCount);
    renderModels->FreeRenderModel(openvrModel);
    geometry.available = !geometry.vertices.empty() && !geometry.indices.empty();
#else
    (void)renderModelName;
#endif

    return geometry;
}

} // namespace ovtr
