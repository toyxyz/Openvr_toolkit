#pragma once

#include "vr/IVRProvider.h"

#include <array>
#include <cstdint>
#include <string>

namespace ovtr::win32 {

struct AppDeviceState;
struct AppOriginState;
struct AppRuntimeState;
struct AppWindowState;

enum class ListDeviceSelectionChange {
    Selected,
    Cleared,
};

std::wstring formatOriginPanelPosition(const AppOriginState& state);
std::wstring formatOriginPanelRotation(const AppOriginState& state);
std::wstring formatOriginEditorText(const AppOriginState& state);
std::wstring formatOriginStepperValue(float value);
std::wstring formatOriginPanelPosition(const AppWindowState& state);
std::wstring formatOriginPanelRotation(const AppWindowState& state);
std::wstring formatOriginEditorText(const AppWindowState& state);

bool parseOriginEditorText(
    const std::wstring& text,
    std::array<float, 3>& offset,
    std::array<float, 3>& rotation
);

bool originValuesAreZero(
    const std::array<float, 3>& offset,
    const std::array<float, 3>& rotation
) noexcept;

const ovtr::PoseSample* poseForRuntimeIndex(
    const ovtr::PosePollResult& poses,
    std::uint32_t runtimeIndex
) noexcept;

bool isPoseValid(const ovtr::PoseSample& pose) noexcept;

ovtr::PosePollResult applyOriginToPoses(
    ovtr::PosePollResult poses,
    bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees
);
bool clearMissingDeviceSelection(
    const AppRuntimeState& runtimeState,
    AppDeviceState& deviceState
);
bool clearMissingDeviceSelection(AppWindowState& state);
ListDeviceSelectionChange toggleListDeviceSelectionState(
    AppDeviceState& state,
    const ovtr::DeviceDescriptor& device
);
ListDeviceSelectionChange toggleListDeviceSelectionState(
    AppWindowState& state,
    const ovtr::DeviceDescriptor& device
);
void ensureOriginSelection(const AppRuntimeState& runtimeState, AppOriginState& originState);
void ensureOriginSelection(AppWindowState& state);
std::string selectNextOriginDevice(const AppRuntimeState& runtimeState, AppOriginState& originState);
std::string selectNextOriginDevice(AppWindowState& state);
bool setOriginFromDevicePose(
    const AppRuntimeState& runtimeState,
    AppOriginState& originState,
    const ovtr::DeviceDescriptor& selected
);
bool setOriginFromDevicePose(AppWindowState& state, const ovtr::DeviceDescriptor& selected);
void clearOriginState(AppOriginState& state);
void clearOriginState(AppWindowState& state);
bool adjustOriginAxis(AppOriginState& state, bool rotation, int axis, float delta);
bool adjustOriginAxis(AppWindowState& state, bool rotation, int axis, float delta);

} // namespace ovtr::win32
