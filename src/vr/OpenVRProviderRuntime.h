#pragma once

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>
#endif

#include <utility>

namespace ovtr {

class OpenVRProviderRuntime final {
public:
    OpenVRProviderRuntime() = default;

#ifdef OVTR_HAS_OPENVR_SDK
    explicit OpenVRProviderRuntime(vr::IVRSystem* system) noexcept
        : system_(system)
    {
    }
#endif

    ~OpenVRProviderRuntime()
    {
        reset();
    }

    OpenVRProviderRuntime(const OpenVRProviderRuntime&) = delete;
    OpenVRProviderRuntime& operator=(const OpenVRProviderRuntime&) = delete;

    OpenVRProviderRuntime(OpenVRProviderRuntime&& other) noexcept
#ifdef OVTR_HAS_OPENVR_SDK
        : system_(std::exchange(other.system_, nullptr))
#endif
    {
    }

    OpenVRProviderRuntime& operator=(OpenVRProviderRuntime&& other) noexcept
    {
        if (this != &other) {
#ifdef OVTR_HAS_OPENVR_SDK
            reset(std::exchange(other.system_, nullptr));
#else
            reset();
#endif
        }
        return *this;
    }

#ifdef OVTR_HAS_OPENVR_SDK
    vr::IVRSystem* system() const noexcept
    {
        return system_;
    }
#endif

    explicit operator bool() const noexcept
    {
#ifdef OVTR_HAS_OPENVR_SDK
        return system_ != nullptr;
#else
        return false;
#endif
    }

#ifdef OVTR_HAS_OPENVR_SDK
    void reset(vr::IVRSystem* system = nullptr) noexcept
    {
        if (system_ && system_ != system) {
            vr::VR_Shutdown();
        }
        system_ = system;
    }
#else
    void reset() noexcept {}
#endif

private:
#ifdef OVTR_HAS_OPENVR_SDK
    vr::IVRSystem* system_ = nullptr;
#endif
};

} // namespace ovtr
