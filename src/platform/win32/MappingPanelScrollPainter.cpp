#include "platform/win32/MappingPanelScrollPainter.h"

#include "platform/win32/AppState.h"
#include "platform/win32/MappingActorLayout.h"
#include "platform/win32/MappingPanelLayout.h"
#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {

void drawScrollbar(HDC drawDc, const RECT& tableRect, const int visibleRows, const int scrollOffset)
{
    const int maxOffset = maxMappingScrollOffset(visibleRows);
    if (maxOffset <= 0) {
        return;
    }

    RECT track{tableRect.right - 8, tableRect.top, tableRect.right, tableRect.bottom};
    UniqueBrush trackBrush(CreateSolidBrush(RGB(34, 38, 47)));
    FillRect(drawDc, &track, trackBrush.get());

    const int trackHeight = track.bottom - track.top;
    int thumbHeight = (trackHeight * visibleRows) / (visibleRows + maxOffset);
    if (thumbHeight < 18) {
        thumbHeight = 18;
    }
    if (thumbHeight > trackHeight) {
        thumbHeight = trackHeight;
    }

    const int thumbTop = track.top + (scrollOffset * (trackHeight - thumbHeight)) / maxOffset;
    RECT thumb{track.left, thumbTop, track.right, thumbTop + thumbHeight};
    UniqueBrush thumbBrush(CreateSolidBrush(RGB(88, 101, 123)));
    FillRect(drawDc, &thumb, thumbBrush.get());
}

void drawActorScrollbar(HDC drawDc, const MappingPanelControlsLayout& controls, const AppWindowState& state)
{
    const int visibleRows = visibleMappingActorRowCount(controls);
    const int maxOffset = maxMappingActorScrollOffset(state.mappingActors.size(), visibleRows);
    if (maxOffset <= 0) {
        return;
    }

    RECT track{controls.actorListRect.right - 8, controls.actorListRect.top,
               controls.actorListRect.right, controls.actorListRect.bottom};
    UniqueBrush trackBrush(CreateSolidBrush(RGB(34, 38, 47)));
    FillRect(drawDc, &track, trackBrush.get());

    const int trackHeight = track.bottom - track.top;
    int thumbHeight = (trackHeight * visibleRows) / (visibleRows + maxOffset);
    if (thumbHeight < 18) {
        thumbHeight = 18;
    }
    const int top = track.top + (state.mappingActorScrollOffset * (trackHeight - thumbHeight)) / maxOffset;
    RECT thumb{track.left, top, track.right, top + thumbHeight};
    UniqueBrush thumbBrush(CreateSolidBrush(RGB(88, 101, 123)));
    FillRect(drawDc, &thumb, thumbBrush.get());
}

} // namespace ovtr::win32
