#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <utility>

namespace ovtr::win32 {

class WindowDc {
public:
    explicit WindowDc(const HWND hwnd) noexcept
        : hwnd_(hwnd)
        , dc_(hwnd ? GetDC(hwnd) : nullptr)
    {
    }

    ~WindowDc()
    {
        if (hwnd_ && dc_) {
            ReleaseDC(hwnd_, dc_);
        }
    }

    WindowDc(const WindowDc&) = delete;
    WindowDc& operator=(const WindowDc&) = delete;

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

private:
    HWND hwnd_ = nullptr;
    HDC dc_ = nullptr;
};

class PaintDc {
public:
    explicit PaintDc(const HWND hwnd) noexcept
        : hwnd_(hwnd)
        , dc_(hwnd ? BeginPaint(hwnd, &paint_) : nullptr)
    {
    }

    ~PaintDc()
    {
        if (hwnd_ && dc_) {
            EndPaint(hwnd_, &paint_);
        }
    }

    PaintDc(const PaintDc&) = delete;
    PaintDc& operator=(const PaintDc&) = delete;

    HDC get() const noexcept
    {
        return dc_;
    }

    PAINTSTRUCT& paint() noexcept
    {
        return paint_;
    }

    explicit operator bool() const noexcept
    {
        return dc_ != nullptr;
    }

private:
    HWND hwnd_ = nullptr;
    PAINTSTRUCT paint_{};
    HDC dc_ = nullptr;
};

} // namespace ovtr::win32
