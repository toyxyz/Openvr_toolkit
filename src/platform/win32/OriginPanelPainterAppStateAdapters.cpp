#include "platform/win32/OriginPanelPainter.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

void paintOriginPanel(
    HDC drawDc,
    HFONT labelFont,
    HFONT valueFont,
    const OriginPanelLayout& layout,
    const AppWindowState& state
)
{
    paintOriginPanel(
        drawDc,
        labelFont,
        valueFont,
        layout,
        static_cast<const AppOriginState&>(state)
    );
}

} // namespace ovtr::win32
