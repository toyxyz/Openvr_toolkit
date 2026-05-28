#pragma once

#include <gl/GL.h>

namespace ovtr::win32 {

class ScopedGlTexture2DBinding {
public:
    explicit ScopedGlTexture2DBinding(const GLuint texture) noexcept
    {
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture_);
        glBindTexture(GL_TEXTURE_2D, texture);
    }

    ~ScopedGlTexture2DBinding()
    {
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture_));
    }

    ScopedGlTexture2DBinding(const ScopedGlTexture2DBinding&) = delete;
    ScopedGlTexture2DBinding& operator=(const ScopedGlTexture2DBinding&) = delete;

private:
    GLint previousTexture_ = 0;
};

} // namespace ovtr::win32
