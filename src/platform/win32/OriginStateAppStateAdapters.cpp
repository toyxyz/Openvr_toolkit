#include "platform/win32/OriginState.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

std::wstring formatOriginPanelPosition(const AppWindowState& state)
{
    return formatOriginPanelPosition(static_cast<const AppOriginState&>(state));
}

std::wstring formatOriginPanelRotation(const AppWindowState& state)
{
    return formatOriginPanelRotation(static_cast<const AppOriginState&>(state));
}

std::wstring formatOriginEditorText(const AppWindowState& state)
{
    return formatOriginEditorText(static_cast<const AppOriginState&>(state));
}

bool clearMissingDeviceSelection(AppWindowState& state)
{
    return clearMissingDeviceSelection(
        static_cast<const AppRuntimeState&>(state),
        static_cast<AppDeviceState&>(state)
    );
}

ListDeviceSelectionChange toggleListDeviceSelectionState(
    AppWindowState& state,
    const ovtr::DeviceDescriptor& device
)
{
    return toggleListDeviceSelectionState(static_cast<AppDeviceState&>(state), device);
}

void ensureOriginSelection(AppWindowState& state)
{
    ensureOriginSelection(
        static_cast<const AppRuntimeState&>(state),
        static_cast<AppOriginState&>(state)
    );
}

std::string selectNextOriginDevice(AppWindowState& state)
{
    return selectNextOriginDevice(
        static_cast<const AppRuntimeState&>(state),
        static_cast<AppOriginState&>(state)
    );
}

bool setOriginFromDevicePose(AppWindowState& state, const ovtr::DeviceDescriptor& selected)
{
    return setOriginFromDevicePose(
        static_cast<const AppRuntimeState&>(state),
        static_cast<AppOriginState&>(state),
        selected
    );
}

void clearOriginState(AppWindowState& state)
{
    clearOriginState(static_cast<AppOriginState&>(state));
}

bool adjustOriginAxis(
    AppWindowState& state,
    const bool rotation,
    const int axis,
    const float delta
)
{
    return adjustOriginAxis(static_cast<AppOriginState&>(state), rotation, axis, delta);
}

} // namespace ovtr::win32
