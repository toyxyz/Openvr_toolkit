#pragma once

#include "platform/win32/ViewportRenderModelTypes.h"

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>
#endif

namespace ovtr::win32 {

#ifdef OVTR_HAS_OPENVR_SDK
bool uploadRenderModelTexture(
    RenderModelMesh& mesh,
    vr::IVRRenderModels* renderModels,
    vr::TextureID_t textureId
);
#else
bool uploadRenderModelTexture(RenderModelMesh& mesh, void* renderModels, int textureId);
#endif

} // namespace ovtr::win32
