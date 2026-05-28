#include "platform/win32/Layout.h"

namespace ovtr::win32 {
namespace {

constexpr int kTopBarHeight = 32;
constexpr int kTopMenuSettingWidth = 92;
constexpr int kTopMenuFileWidth = 68;
constexpr int kTopMenuGap = 6;

} // namespace

RECT topBarSettingRectForClient(const int clientWidth, const int clientHeight) noexcept
{
    if (clientWidth <= 0 || clientHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    const int top = 4;
    const int bottom = clientHeight < kTopBarHeight ? clientHeight : kTopBarHeight;
    const int buttonBottom = bottom > top ? bottom - 4 : bottom;
    const int left = 8 + kTopMenuFileWidth + kTopMenuGap;
    if (left >= clientWidth) {
        return RECT{0, 0, 0, 0};
    }
    const int right = left + kTopMenuSettingWidth < clientWidth
        ? left + kTopMenuSettingWidth
        : clientWidth;
    return RECT{left, top, right, buttonBottom};
}

RECT topBarFileRectForClient(const int clientWidth, const int clientHeight) noexcept
{
    if (clientWidth <= 8 || clientHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    const int top = 4;
    const int bottom = clientHeight < kTopBarHeight ? clientHeight : kTopBarHeight;
    const int buttonBottom = bottom > top ? bottom - 4 : bottom;
    const int right = 8 + kTopMenuFileWidth < clientWidth
        ? 8 + kTopMenuFileWidth
        : clientWidth;
    return RECT{8, top, right, buttonBottom};
}

} // namespace ovtr::win32
