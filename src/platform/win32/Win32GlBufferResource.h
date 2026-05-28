#pragma once

#include <glad/glad.h>

#include <utility>

namespace ovtr::win32 {

class UniqueGlBuffer {
public:
    UniqueGlBuffer() = default;

    explicit UniqueGlBuffer(const GLuint buffer) noexcept
        : buffer_(buffer)
    {
    }

    ~UniqueGlBuffer()
    {
        reset();
    }

    UniqueGlBuffer(const UniqueGlBuffer&) = delete;
    UniqueGlBuffer& operator=(const UniqueGlBuffer&) = delete;

    UniqueGlBuffer(UniqueGlBuffer&& other) noexcept
        : buffer_(std::exchange(other.buffer_, 0))
    {
    }

    UniqueGlBuffer& operator=(UniqueGlBuffer&& other) noexcept
    {
        if (this != &other) {
            reset(std::exchange(other.buffer_, 0));
        }
        return *this;
    }

    GLuint get() const noexcept
    {
        return buffer_;
    }

    explicit operator bool() const noexcept
    {
        return buffer_ != 0;
    }

    GLuint release() noexcept
    {
        return std::exchange(buffer_, 0);
    }

    void reset(const GLuint buffer = 0) noexcept
    {
        if (buffer_ != 0 && buffer_ != buffer && glad_glDeleteBuffers != nullptr) {
            glad_glDeleteBuffers(1, &buffer_);
        }
        buffer_ = buffer;
    }

private:
    GLuint buffer_ = 0;
};

} // namespace ovtr::win32
