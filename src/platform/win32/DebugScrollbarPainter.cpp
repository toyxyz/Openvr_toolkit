#include "platform/win32/DebugScrollbarPainter.h"

#include "platform/win32/Layout.h"
#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {

void paintDebugScrollbar(
    HDC drawDc,
    const RECT& bodyRect,
    const int totalLineCount,
    const int visibleLineCount,
    const int scrollOffset,
    const bool reverseScrollOrigin
)
{
    const DebugScrollbarLayout scrollbar = debugScrollbarLayoutForRect(
        bodyRect,
        totalLineCount,
        visibleLineCount,
        scrollOffset,
        reverseScrollOrigin
    );
    if (!scrollbar.valid) {
        return;
    }

    UniqueBrush trackBrush(CreateSolidBrush(RGB(34, 38, 47)));
    FillRect(drawDc, &scrollbar.trackRect, trackBrush.get());
    UniqueBrush thumbBrush(CreateSolidBrush(RGB(88, 101, 123)));
    FillRect(drawDc, &scrollbar.thumbRect, thumbBrush.get());
}

} // namespace ovtr::win32
