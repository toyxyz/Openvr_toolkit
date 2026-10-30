#include "platform/win32/DevicePanelScrollbarPainter.h"
#include "platform/win32/UiTheme.h"

#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {

void paintDeviceListScrollbar(
    HDC drawDc,
    const DeviceListLayout& layout,
    const int totalDeviceRows,
    const int maxScrollOffset,
    const int scrollOffset
)
{
    RECT scrollbarTrack{
        layout.contentRect.right - 8,
        layout.contentRect.top,
        layout.contentRect.right,
        layout.contentRect.bottom
    };
    UniqueBrush trackBrush(CreateSolidBrush(ui::timeline));
    FillRect(drawDc, &scrollbarTrack, trackBrush.get());

    const int trackHeight = scrollbarTrack.bottom - scrollbarTrack.top;
    int thumbHeight = (trackHeight * layout.visibleItemCount) / totalDeviceRows;
    if (thumbHeight < 18) {
        thumbHeight = 18;
    }
    if (thumbHeight > trackHeight) {
        thumbHeight = trackHeight;
    }

    const int travel = trackHeight - thumbHeight;
    const int thumbTop = scrollbarTrack.top +
        (maxScrollOffset > 0 ? (scrollOffset * travel) / maxScrollOffset : 0);
    RECT thumbRect{scrollbarTrack.left, thumbTop, scrollbarTrack.right, thumbTop + thumbHeight};
    UniqueBrush thumbBrush(CreateSolidBrush(ui::scrollbar));
    FillRect(drawDc, &thumbRect, thumbBrush.get());
}

} // namespace ovtr::win32
