#pragma once

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>
#endif

#include <utility>

namespace ovtr {

#ifdef OVTR_HAS_OPENVR_SDK
class OpenVRRenderModelHandle {
public:
    OpenVRRenderModelHandle() = default;

    OpenVRRenderModelHandle(vr::IVRRenderModels* renderModels, vr::RenderModel_t* model = nullptr) noexcept
        : renderModels_(renderModels)
        , model_(model)
    {
    }

    ~OpenVRRenderModelHandle()
    {
        reset();
    }

    OpenVRRenderModelHandle(const OpenVRRenderModelHandle&) = delete;
    OpenVRRenderModelHandle& operator=(const OpenVRRenderModelHandle&) = delete;

    OpenVRRenderModelHandle(OpenVRRenderModelHandle&& other) noexcept
        : renderModels_(std::exchange(other.renderModels_, nullptr))
        , model_(std::exchange(other.model_, nullptr))
    {
    }

    OpenVRRenderModelHandle& operator=(OpenVRRenderModelHandle&& other) noexcept
    {
        if (this != &other) {
            reset();
            renderModels_ = std::exchange(other.renderModels_, nullptr);
            model_ = std::exchange(other.model_, nullptr);
        }
        return *this;
    }

    vr::RenderModel_t** output() noexcept
    {
        return &model_;
    }

    vr::RenderModel_t* get() const noexcept
    {
        return model_;
    }

    vr::RenderModel_t* operator->() const noexcept
    {
        return model_;
    }

    void reset(vr::RenderModel_t* model = nullptr) noexcept
    {
        if (model_ && renderModels_) {
            renderModels_->FreeRenderModel(model_);
        }
        model_ = model;
    }

private:
    vr::IVRRenderModels* renderModels_ = nullptr;
    vr::RenderModel_t* model_ = nullptr;
};
#endif

} // namespace ovtr
