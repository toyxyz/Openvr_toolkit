#pragma once

#include <gl/GL.h>

namespace ovtr::win32 {

class ScopedGlMatrixStack {
public:
    explicit ScopedGlMatrixStack(const GLenum mode) noexcept
        : mode_(mode)
    {
        glGetIntegerv(GL_MATRIX_MODE, &previousMode_);
        glMatrixMode(mode_);
        glPushMatrix();
    }

    ~ScopedGlMatrixStack()
    {
        glMatrixMode(mode_);
        glPopMatrix();
        glMatrixMode(static_cast<GLenum>(previousMode_));
    }

    ScopedGlMatrixStack(const ScopedGlMatrixStack&) = delete;
    ScopedGlMatrixStack& operator=(const ScopedGlMatrixStack&) = delete;

private:
    GLenum mode_;
    GLint previousMode_ = GL_MODELVIEW;
};

} // namespace ovtr::win32
