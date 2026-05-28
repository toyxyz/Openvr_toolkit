#include "platform/win32/ViewportSceneRenderer.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

void drawTrackedDevices3D(AppWindowState& state, const int viewportHeight)
{
    drawTrackedDevices3D(
        static_cast<AppRuntimeState&>(state),
        static_cast<const AppDeviceState&>(state),
        static_cast<const AppOriginState&>(state),
        static_cast<AppViewportState&>(state),
        viewportHeight
    );
}

void drawTrackedDeviceLabels3D(const AppWindowState& state)
{
    drawTrackedDeviceLabels3D(
        static_cast<const AppRuntimeState&>(state),
        static_cast<const AppDeviceState&>(state),
        static_cast<const AppOriginState&>(state),
        static_cast<const AppViewportState&>(state)
    );
}

} // namespace ovtr::win32
