#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <gl/GL.h>

#include <utility>

namespace ovtr::win32 {

class UniqueGlDisplayList {
public:
    UniqueGlDisplayList() = default;

    UniqueGlDisplayList(const GLuint base, const GLsizei range) noexcept
        : base_(base)
        , range_(range)
    {
    }

    ~UniqueGlDisplayList()
    {
        reset();
    }

    UniqueGlDisplayList(const UniqueGlDisplayList&) = delete;
    UniqueGlDisplayList& operator=(const UniqueGlDisplayList&) = delete;

    UniqueGlDisplayList(UniqueGlDisplayList&& other) noexcept
        : base_(std::exchange(other.base_, 0))
        , range_(std::exchange(other.range_, 0))
    {
    }

    UniqueGlDisplayList& operator=(UniqueGlDisplayList&& other) noexcept
    {
        if (this != &other) {
            const GLuint base = std::exchange(other.base_, 0);
            const GLsizei range = std::exchange(other.range_, 0);
            reset(base, range);
        }
        return *this;
    }

    GLuint get() const noexcept
    {
        return base_;
    }

    explicit operator bool() const noexcept
    {
        return base_ != 0;
    }

    GLuint release() noexcept
    {
        range_ = 0;
        return std::exchange(base_, 0);
    }

    void reset(const GLuint base = 0, const GLsizei range = 0) noexcept
    {
        if (base_ != 0 && (base_ != base || range_ != range)) {
            glDeleteLists(base_, range_);
        }
        base_ = base;
        range_ = range;
    }

private:
    GLuint base_ = 0;
    GLsizei range_ = 0;
};

} // namespace ovtr::win32
