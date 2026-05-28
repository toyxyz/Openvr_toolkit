#include "TestCases.h"
#include "TestSupport.h"

#include "platform/win32/AppOriginState.h"
#include "platform/win32/OriginDialogModel.h"
#include "platform/win32/OriginState.h"

namespace ovtr::test {

void testWin32OriginDialogState()
{
    ovtr::win32::AppOriginState state;
    ovtr::DeviceDescriptor controller;
    controller.runtimeIndex = 7;
    controller.deviceClass = ovtr::DeviceClass::Controller;
    controller.serial = "CTRL";

    ovtr::win32::OriginDialogValues dialogValues;
    dialogValues.enabled = true;
    dialogValues.offset = {0.0f, 0.0f, 0.0f};
    dialogValues.rotationDegrees = {0.0f, 0.0f, 0.0f};
    state.selectedOriginRuntimeIndex = controller.runtimeIndex;
    ovtr::win32::applyOriginDialogValuesToState(state, dialogValues);
    require(!state.originEnabled, "origin dialog zero values disable origin");
    require(state.originOffset == std::array<float, 3>{0.0f, 0.0f, 0.0f}, "origin dialog zero clears offset");
    require(
        state.selectedOriginRuntimeIndex == ovtr::win32::kNoSelectedRuntimeIndex,
        "origin dialog apply clears selected origin device"
    );

    dialogValues.offset = {1.0f, 2.0f, 3.0f};
    dialogValues.rotationDegrees = {4.0f, 5.0f, 6.0f};
    ovtr::win32::applyOriginDialogValuesToState(state, dialogValues);
    require(state.originEnabled, "origin dialog non-zero values enable origin");
    require(state.originOffset == dialogValues.offset, "origin dialog stores offset");
    require(state.originRotationDegrees == dialogValues.rotationDegrees, "origin dialog stores rotation");
    require(
        ovtr::win32::originDialogValuesFromState(state).rotationDegrees == dialogValues.rotationDegrees,
        "origin dialog snapshots state rotation"
    );

    dialogValues.enabled = false;
    dialogValues.offset = {7.0f, 8.0f, 9.0f};
    dialogValues.rotationDegrees = {10.0f, 11.0f, 12.0f};
    ovtr::win32::applyOriginDialogValuesToState(state, dialogValues);
    require(!state.originEnabled, "origin dialog unchecked disables origin");
    require(state.originOffset == dialogValues.offset, "origin dialog disabled state preserves offset");
    require(
        state.originRotationDegrees == dialogValues.rotationDegrees,
        "origin dialog disabled state preserves rotation"
    );

    ovtr::win32::clearOriginState(state);
    require(!state.originEnabled, "clear origin disables origin");
    require(state.originOffset == std::array<float, 3>{0.0f, 0.0f, 0.0f}, "clear origin clears offset");
    require(state.originStatusMessage == "origin cleared", "clear origin status");
}

} // namespace ovtr::test
