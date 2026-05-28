#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <utility>

namespace ovtr::win32 {

class UniqueWindowDc {
public:
    UniqueWindowDc() = default;

    UniqueWindowDc(HWND hwnd, HDC dc) noexcept
        : hwnd_(hwnd)
        , dc_(dc)
    {
    }

    ~UniqueWindowDc()
    {
        reset();
    }

    UniqueWindowDc(const UniqueWindowDc&) = delete;
    UniqueWindowDc& operator=(const UniqueWindowDc&) = delete;

    UniqueWindowDc(UniqueWindowDc&& other) noexcept
        : hwnd_(std::exchange(other.hwnd_, nullptr))
        , dc_(std::exchange(other.dc_, nullptr))
    {
    }

    UniqueWindowDc& operator=(UniqueWindowDc&& other) noexcept
    {
        if (this != &other) {
            const HWND hwnd = std::exchange(other.hwnd_, nullptr);
            const HDC dc = std::exchange(other.dc_, nullptr);
            reset(hwnd, dc);
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
        hwnd_ = nullptr;
        return std::exchange(dc_, nullptr);
    }

    void reset(HWND hwnd = nullptr, HDC dc = nullptr) noexcept
    {
        if (dc_ && (hwnd_ != hwnd || dc_ != dc)) {
            ReleaseDC(hwnd_, dc_);
        }
        hwnd_ = hwnd;
        dc_ = dc;
    }

private:
    HWND hwnd_ = nullptr;
    HDC dc_ = nullptr;
};

} // namespace ovtr::win32
