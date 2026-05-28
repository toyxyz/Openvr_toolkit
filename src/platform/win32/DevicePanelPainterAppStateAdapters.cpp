#include "platform/win32/DevicePanelPainter.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

void paintDeviceListPanel(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    AppWindowState& state,
    const DeviceListLayout& layout
)
{
    paintDeviceListPanel(
        drawDc,
        bodyFont,
        headerFont,
        static_cast<const AppRuntimeState&>(state),
        static_cast<AppDeviceState&>(state),
        layout
    );
}

} // namespace ovtr::win32
