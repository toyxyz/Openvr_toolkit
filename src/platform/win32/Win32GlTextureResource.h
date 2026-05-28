#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <gl/GL.h>

#include <utility>

namespace ovtr::win32 {

class UniqueGlTexture {
public:
    UniqueGlTexture() = default;

    explicit UniqueGlTexture(const GLuint texture) noexcept
        : texture_(texture)
    {
    }

    ~UniqueGlTexture()
    {
        reset();
    }

    UniqueGlTexture(const UniqueGlTexture&) = delete;
    UniqueGlTexture& operator=(const UniqueGlTexture&) = delete;

    UniqueGlTexture(UniqueGlTexture&& other) noexcept
        : texture_(std::exchange(other.texture_, 0))
    {
    }

    UniqueGlTexture& operator=(UniqueGlTexture&& other) noexcept
    {
        if (this != &other) {
            reset(std::exchange(other.texture_, 0));
        }
        return *this;
    }

    GLuint get() const noexcept
    {
        return texture_;
    }

    explicit operator bool() const noexcept
    {
        return texture_ != 0;
    }

    GLuint release() noexcept
    {
        return std::exchange(texture_, 0);
    }

    void reset(const GLuint texture = 0) noexcept
    {
        if (texture_ != 0 && texture_ != texture) {
            glDeleteTextures(1, &texture_);
        }
        texture_ = texture;
    }

private:
    GLuint texture_ = 0;
};

} // namespace ovtr::win32
