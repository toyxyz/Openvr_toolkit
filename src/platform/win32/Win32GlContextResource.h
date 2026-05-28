#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <gl/GL.h>

#include <utility>

namespace ovtr::win32 {

class UniqueGlContext {
public:
    UniqueGlContext() = default;

    explicit UniqueGlContext(const HGLRC context) noexcept
        : context_(context)
    {
    }

    ~UniqueGlContext()
    {
        reset();
    }

    UniqueGlContext(const UniqueGlContext&) = delete;
    UniqueGlContext& operator=(const UniqueGlContext&) = delete;

    UniqueGlContext(UniqueGlContext&& other) noexcept
        : context_(std::exchange(other.context_, nullptr))
    {
    }

    UniqueGlContext& operator=(UniqueGlContext&& other) noexcept
    {
        if (this != &other) {
            reset(std::exchange(other.context_, nullptr));
        }
        return *this;
    }

    HGLRC get() const noexcept
    {
        return context_;
    }

    explicit operator bool() const noexcept
    {
        return context_ != nullptr;
    }

    HGLRC release() noexcept
    {
        return std::exchange(context_, nullptr);
    }

    void reset(const HGLRC context = nullptr) noexcept
    {
        if (context_ && context_ != context) {
            wglDeleteContext(context_);
        }
        context_ = context;
    }

private:
    HGLRC context_ = nullptr;
};

} // namespace ovtr::win32
