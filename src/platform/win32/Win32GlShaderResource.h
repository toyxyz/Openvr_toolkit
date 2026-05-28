#pragma once

#include <glad/glad.h>

#include <utility>

namespace ovtr::win32 {

class UniqueGlShader {
public:
    UniqueGlShader() = default;

    explicit UniqueGlShader(const GLuint shader) noexcept
        : shader_(shader)
    {
    }

    ~UniqueGlShader()
    {
        reset();
    }

    UniqueGlShader(const UniqueGlShader&) = delete;
    UniqueGlShader& operator=(const UniqueGlShader&) = delete;

    UniqueGlShader(UniqueGlShader&& other) noexcept
        : shader_(std::exchange(other.shader_, 0))
    {
    }

    UniqueGlShader& operator=(UniqueGlShader&& other) noexcept
    {
        if (this != &other) {
            reset(std::exchange(other.shader_, 0));
        }
        return *this;
    }

    GLuint get() const noexcept
    {
        return shader_;
    }

    explicit operator bool() const noexcept
    {
        return shader_ != 0;
    }

    GLuint release() noexcept
    {
        return std::exchange(shader_, 0);
    }

    void reset(const GLuint shader = 0) noexcept
    {
        if (shader_ != 0 && shader_ != shader && glad_glDeleteShader != nullptr) {
            glad_glDeleteShader(shader_);
        }
        shader_ = shader;
    }

private:
    GLuint shader_ = 0;
};

class UniqueGlProgram {
public:
    UniqueGlProgram() = default;

    explicit UniqueGlProgram(const GLuint program) noexcept
        : program_(program)
    {
    }

    ~UniqueGlProgram()
    {
        reset();
    }

    UniqueGlProgram(const UniqueGlProgram&) = delete;
    UniqueGlProgram& operator=(const UniqueGlProgram&) = delete;

    UniqueGlProgram(UniqueGlProgram&& other) noexcept
        : program_(std::exchange(other.program_, 0))
    {
    }

    UniqueGlProgram& operator=(UniqueGlProgram&& other) noexcept
    {
        if (this != &other) {
            reset(std::exchange(other.program_, 0));
        }
        return *this;
    }

    GLuint get() const noexcept
    {
        return program_;
    }

    explicit operator bool() const noexcept
    {
        return program_ != 0;
    }

    GLuint release() noexcept
    {
        return std::exchange(program_, 0);
    }

    void reset(const GLuint program = 0) noexcept
    {
        if (program_ != 0 && program_ != program && glad_glDeleteProgram != nullptr) {
            glad_glDeleteProgram(program_);
        }
        program_ = program;
    }

private:
    GLuint program_ = 0;
};

} // namespace ovtr::win32
