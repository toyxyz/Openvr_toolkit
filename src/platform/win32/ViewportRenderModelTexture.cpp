#include "platform/win32/ViewportRenderModelTexture.h"

#include "platform/win32/OpenVrRenderModelHandles.h"
#include "platform/win32/ViewportGlTextureBindingScope.h"
#include "platform/win32/Win32GlTextureResource.h"

#include <gl/GL.h>

namespace ovtr::win32 {

bool uploadRenderModelTexture(
    RenderModelMesh& mesh,
#ifdef OVTR_HAS_OPENVR_SDK
    vr::IVRRenderModels* renderModels,
    const vr::TextureID_t textureId
#else
    void*,
    const int
#endif
)
{
    if (mesh.texture || mesh.textureAvailable) {
        return true;
    }
    if (mesh.textureLoadFailed) {
        return false;
    }

#ifdef OVTR_HAS_OPENVR_SDK
    if (renderModels == nullptr || textureId < 0) {
        mesh.textureLoadFailed = true;
        return false;
    }

    vr::RenderModel_TextureMap_t* rawTexture = nullptr;
    const vr::EVRRenderModelError error = renderModels->LoadTexture_Async(textureId, &rawTexture);
    if (error == vr::VRRenderModelError_Loading) {
        return false;
    }
    OpenVrTextureHandle texture(renderModels, rawTexture);
    if (error != vr::VRRenderModelError_None || texture.get() == nullptr || texture->rubTextureMapData == nullptr) {
        mesh.textureLoadFailed = true;
        return false;
    }
    if (texture->format != vr::VRRenderModelTextureFormat_RGBA8_SRGB ||
        texture->unWidth == 0 ||
        texture->unHeight == 0) {
        mesh.textureLoadFailed = true;
        return false;
    }

    GLuint glTexture = 0;
    glGenTextures(1, &glTexture);
    if (glTexture == 0) {
        mesh.textureLoadFailed = true;
        return false;
    }
    UniqueGlTexture textureHandle(glTexture);

    {
        ScopedGlTexture2DBinding textureBinding(textureHandle.get());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            texture->unWidth,
            texture->unHeight,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            texture->rubTextureMapData
        );
    }

    mesh.texture.reset(textureHandle.release());
    mesh.textureAvailable = true;
    mesh.textureLoadFailed = false;
    return true;
#else
    mesh.textureLoadFailed = true;
    return false;
#endif
}

} // namespace ovtr::win32
