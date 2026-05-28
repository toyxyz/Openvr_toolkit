#include "platform/win32/ViewportRenderer.h"

#include "platform/win32/OpenVrRenderModelHandles.h"
#include "platform/win32/ViewportRenderModelTypes.h"

#include <cstdint>

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>
#endif

namespace ovtr::win32 {

bool updateRenderModelMesh(RenderModelMesh& mesh, const std::string& renderModelName)
{
    if (mesh.state == RenderModelMesh::LoadState::Failed || renderModelName.empty()) {
        return false;
    }

#ifdef OVTR_HAS_OPENVR_SDK
    vr::IVRRenderModels* renderModels = vr::VRRenderModels();
    if (renderModels == nullptr) {
        if (mesh.state != RenderModelMesh::LoadState::Ready) {
            mesh.state = RenderModelMesh::LoadState::Failed;
            return false;
        }
        return true;
    }

    if (mesh.state == RenderModelMesh::LoadState::Ready) {
        return true;
    }

    vr::RenderModel_t* rawOpenvrModel = nullptr;
    const vr::EVRRenderModelError error =
        renderModels->LoadRenderModel_Async(renderModelName.c_str(), &rawOpenvrModel);
    if (error == vr::VRRenderModelError_Loading) {
        return false;
    }
    OpenVrRenderModelHandle openvrModel(renderModels, rawOpenvrModel);
    if (error != vr::VRRenderModelError_None || openvrModel.get() == nullptr) {
        mesh.state = RenderModelMesh::LoadState::Failed;
        return false;
    }

    mesh.vertices.clear();
    mesh.vertices.reserve(openvrModel->unVertexCount);
    for (std::uint32_t i = 0; i < openvrModel->unVertexCount; ++i) {
        const vr::RenderModel_Vertex_t& source = openvrModel->rVertexData[i];
        RenderModelVertex vertex;
        vertex.position = {source.vPosition.v[0], source.vPosition.v[1], source.vPosition.v[2]};
        vertex.normal = {source.vNormal.v[0], source.vNormal.v[1], source.vNormal.v[2]};
        vertex.texCoord = {source.rfTextureCoord[0], source.rfTextureCoord[1]};
        mesh.vertices.push_back(vertex);
    }

    const std::uint32_t indexCount = openvrModel->unTriangleCount * 3;
    mesh.indices.assign(openvrModel->rIndexData, openvrModel->rIndexData + indexCount);
    mesh.diffuseTextureId = openvrModel->diffuseTextureId;

    mesh.state = mesh.vertices.empty() || mesh.indices.empty()
        ? RenderModelMesh::LoadState::Failed
        : RenderModelMesh::LoadState::Ready;
    return mesh.state == RenderModelMesh::LoadState::Ready;
#else
    mesh.state = RenderModelMesh::LoadState::Failed;
    return false;
#endif
}

} // namespace ovtr::win32
