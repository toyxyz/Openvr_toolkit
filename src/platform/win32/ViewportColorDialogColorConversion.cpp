#include "platform/win32/ViewportColorDialogSessionInternal.h"

#include "platform/win32/ConfigStore.h"

namespace ovtr::win32 {

COLORREF colorRefFromRgb(const RgbColor color)
{
    const RgbColor clamped = clampRgbColor(color);
    return RGB(clamped.r, clamped.g, clamped.b);
}

RgbColor rgbFromColorRef(const COLORREF color)
{
    return {
        static_cast<int>(GetRValue(color)),
        static_cast<int>(GetGValue(color)),
        static_cast<int>(GetBValue(color)),
    };
}

} // namespace ovtr::win32
