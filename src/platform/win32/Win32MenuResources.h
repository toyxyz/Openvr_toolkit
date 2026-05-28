#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <utility>

namespace ovtr::win32 {

class UniqueMenu {
public:
    UniqueMenu() = default;

    explicit UniqueMenu(const HMENU menu) noexcept
        : menu_(menu)
    {
    }

    ~UniqueMenu()
    {
        reset();
    }

    UniqueMenu(const UniqueMenu&) = delete;
    UniqueMenu& operator=(const UniqueMenu&) = delete;

    UniqueMenu(UniqueMenu&& other) noexcept
        : menu_(std::exchange(other.menu_, nullptr))
    {
    }

    UniqueMenu& operator=(UniqueMenu&& other) noexcept
    {
        if (this != &other) {
            reset(std::exchange(other.menu_, nullptr));
        }
        return *this;
    }

    HMENU get() const noexcept
    {
        return menu_;
    }

    explicit operator bool() const noexcept
    {
        return menu_ != nullptr;
    }

    HMENU release() noexcept
    {
        return std::exchange(menu_, nullptr);
    }

    void reset(const HMENU menu = nullptr) noexcept
    {
        if (menu_ && menu_ != menu) {
            DestroyMenu(menu_);
        }
        menu_ = menu;
    }

private:
    HMENU menu_ = nullptr;
};

} // namespace ovtr::win32
