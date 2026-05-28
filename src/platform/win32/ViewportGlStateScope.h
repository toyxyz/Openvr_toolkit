#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <gl/GL.h>

namespace ovtr::win32 {

class ScopedGlCapability {
public:
    ScopedGlCapability(const GLenum capability, const bool enabled) noexcept
        : capability_(capability),
          wasEnabled_(glIsEnabled(capability) == GL_TRUE)
    {
        if (enabled) {
            glEnable(capability_);
        } else {
            glDisable(capability_);
        }
    }

    ~ScopedGlCapability()
    {
        if (wasEnabled_) {
            glEnable(capability_);
        } else {
            glDisable(capability_);
        }
    }

    ScopedGlCapability(const ScopedGlCapability&) = delete;
    ScopedGlCapability& operator=(const ScopedGlCapability&) = delete;

private:
    GLenum capability_ = GL_DEPTH_TEST;
    bool wasEnabled_ = false;
};

class ScopedGlDepthFunc {
public:
    explicit ScopedGlDepthFunc(const GLenum mode) noexcept
    {
        glGetIntegerv(GL_DEPTH_FUNC, &previousMode_);
        glDepthFunc(mode);
    }

    ~ScopedGlDepthFunc()
    {
        glDepthFunc(static_cast<GLenum>(previousMode_));
    }

    ScopedGlDepthFunc(const ScopedGlDepthFunc&) = delete;
    ScopedGlDepthFunc& operator=(const ScopedGlDepthFunc&) = delete;

private:
    GLint previousMode_ = GL_LESS;
};

class ScopedGlCullFace {
public:
    explicit ScopedGlCullFace(const GLenum mode) noexcept
    {
        glGetIntegerv(GL_CULL_FACE_MODE, &previousMode_);
        glCullFace(mode);
    }

    ~ScopedGlCullFace()
    {
        glCullFace(static_cast<GLenum>(previousMode_));
    }

    ScopedGlCullFace(const ScopedGlCullFace&) = delete;
    ScopedGlCullFace& operator=(const ScopedGlCullFace&) = delete;

private:
    GLint previousMode_ = GL_BACK;
};

class ScopedGlLineWidth {
public:
    explicit ScopedGlLineWidth(const GLfloat width) noexcept
    {
        glGetFloatv(GL_LINE_WIDTH, &previousWidth_);
        glLineWidth(width);
    }

    ~ScopedGlLineWidth()
    {
        glLineWidth(previousWidth_);
    }

    ScopedGlLineWidth(const ScopedGlLineWidth&) = delete;
    ScopedGlLineWidth& operator=(const ScopedGlLineWidth&) = delete;

private:
    GLfloat previousWidth_ = 1.0f;
};

} // namespace ovtr::win32
