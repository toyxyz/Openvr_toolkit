#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <utility>

namespace ovtr::win32 {

class CompatibleDc {
public:
    CompatibleDc() = default;

    explicit CompatibleDc(const HDC sourceDc) noexcept
        : dc_(sourceDc ? CreateCompatibleDC(sourceDc) : nullptr)
    {
    }

    ~CompatibleDc()
    {
        reset();
    }

    CompatibleDc(const CompatibleDc&) = delete;
    CompatibleDc& operator=(const CompatibleDc&) = delete;

    CompatibleDc(CompatibleDc&& other) noexcept
        : dc_(std::exchange(other.dc_, nullptr))
    {
    }

    CompatibleDc& operator=(CompatibleDc&& other) noexcept
    {
        if (this != &other) {
            reset(std::exchange(other.dc_, nullptr));
        }
        return *this;
    }

    HDC get() const noexcept
    {
        return dc_;
    }

    explicit operator bool() const noexcept
    {
        return dc_ != nullptr;
    }

    HDC release() noexcept
    {
        return std::exchange(dc_, nullptr);
    }

    void reset(const HDC dc = nullptr) noexcept
    {
        if (dc_ && dc_ != dc) {
            DeleteDC(dc_);
        }
        dc_ = dc;
    }

private:
    HDC dc_ = nullptr;
};

} // namespace ovtr::win32
