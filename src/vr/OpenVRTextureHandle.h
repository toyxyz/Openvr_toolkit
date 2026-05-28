#pragma once

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>
#endif

#include <utility>

namespace ovtr {

#ifdef OVTR_HAS_OPENVR_SDK
class OpenVRTextureHandle {
public:
    OpenVRTextureHandle() = default;

    OpenVRTextureHandle(vr::IVRRenderModels* renderModels, vr::RenderModel_TextureMap_t* texture = nullptr) noexcept
        : renderModels_(renderModels)
        , texture_(texture)
    {
    }

    ~OpenVRTextureHandle()
    {
        reset();
    }

    OpenVRTextureHandle(const OpenVRTextureHandle&) = delete;
    OpenVRTextureHandle& operator=(const OpenVRTextureHandle&) = delete;

    OpenVRTextureHandle(OpenVRTextureHandle&& other) noexcept
        : renderModels_(std::exchange(other.renderModels_, nullptr))
        , texture_(std::exchange(other.texture_, nullptr))
    {
    }

    OpenVRTextureHandle& operator=(OpenVRTextureHandle&& other) noexcept
    {
        if (this != &other) {
            reset();
            renderModels_ = std::exchange(other.renderModels_, nullptr);
            texture_ = std::exchange(other.texture_, nullptr);
        }
        return *this;
    }

    vr::RenderModel_TextureMap_t* get() const noexcept
    {
        return texture_;
    }

    vr::RenderModel_TextureMap_t* operator->() const noexcept
    {
        return texture_;
    }

    void reset(vr::RenderModel_TextureMap_t* texture = nullptr) noexcept
    {
        if (texture_ && renderModels_) {
            renderModels_->FreeTexture(texture_);
        }
        texture_ = texture;
    }

private:
    vr::IVRRenderModels* renderModels_ = nullptr;
    vr::RenderModel_TextureMap_t* texture_ = nullptr;
};
#endif

} // namespace ovtr
