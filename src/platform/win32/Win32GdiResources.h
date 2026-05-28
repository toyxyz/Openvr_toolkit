#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <utility>

namespace ovtr::win32 {

template <typename Handle>
class UniqueGdiObject {
public:
    UniqueGdiObject() = default;

    explicit UniqueGdiObject(const Handle handle) noexcept
        : handle_(handle)
    {
    }

    ~UniqueGdiObject()
    {
        reset();
    }

    UniqueGdiObject(const UniqueGdiObject&) = delete;
    UniqueGdiObject& operator=(const UniqueGdiObject&) = delete;

    UniqueGdiObject(UniqueGdiObject&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr))
    {
    }

    UniqueGdiObject& operator=(UniqueGdiObject&& other) noexcept
    {
        if (this != &other) {
            reset(std::exchange(other.handle_, nullptr));
        }
        return *this;
    }

    Handle get() const noexcept
    {
        return handle_;
    }

    explicit operator bool() const noexcept
    {
        return handle_ != nullptr;
    }

    Handle release() noexcept
    {
        return std::exchange(handle_, nullptr);
    }

    void reset(const Handle handle = nullptr) noexcept
    {
        if (handle_ && handle_ != handle) {
            DeleteObject(handle_);
        }
        handle_ = handle;
    }

private:
    Handle handle_ = nullptr;
};

using UniqueBrush = UniqueGdiObject<HBRUSH>;
using UniquePen = UniqueGdiObject<HPEN>;
using UniqueFont = UniqueGdiObject<HFONT>;
using UniqueBitmap = UniqueGdiObject<HBITMAP>;

class SelectObjectGuard {
public:
    SelectObjectGuard(HDC dc, HGDIOBJ object) noexcept
        : dc_(dc)
    {
        if (dc_ && object) {
            previous_ = SelectObject(dc_, object);
            if (previous_ == HGDI_ERROR) {
                previous_ = nullptr;
            }
        }
    }

    ~SelectObjectGuard()
    {
        if (dc_ && previous_) {
            SelectObject(dc_, previous_);
        }
    }

    SelectObjectGuard(const SelectObjectGuard&) = delete;
    SelectObjectGuard& operator=(const SelectObjectGuard&) = delete;

private:
    HDC dc_ = nullptr;
    HGDIOBJ previous_ = nullptr;
};

} // namespace ovtr::win32
