#include "TestCases.h"
#include "TestSupport.h"
#include "Win32LayoutTestSupport.h"

#include "platform/win32/AppState.h"
#include "platform/win32/Layout.h"
#include "platform/win32/WindowLayout.h"

namespace ovtr::test {

void testWin32Layout()
{
    require(
        sameRect(ovtr::win32::topBarFileRectForClient(180, 32), 8, 4, 76, 28),
        "file menu rect"
    );
    require(
        sameRect(ovtr::win32::topBarSettingRectForClient(180, 32), 82, 4, 174, 28),
        "settings menu rect"
    );
    require(
        sameRect(ovtr::win32::topBarSettingRectForClient(80, 32), 0, 0, 0, 0),
        "settings menu rect rejects narrow client"
    );

    require(ovtr::win32::defaultLeftPanelWidthForClient(1000) == 420, "default left panel min");
    require(ovtr::win32::defaultLeftPanelWidthForClient(2000) == 560, "default left panel proportional");
    require(ovtr::win32::defaultLeftPanelWidthForClient(3000) == 620, "default left panel max");
    require(ovtr::win32::clampLeftPanelWidthForClient(100, 1200) == 320, "left panel min clamp");
    require(ovtr::win32::clampLeftPanelWidthForClient(900, 1200) == 840, "left panel viewport max clamp");
    require(ovtr::win32::leftPanelWidthForClient(false, 500, 900) == 32, "hidden panel rail width");
    require(ovtr::win32::leftPanelWidthForClient(true, 0, 2000) == 560, "visible panel default width");
    require(
        ovtr::win32::rightProfileAreaWidthForClient(false, 1200) == 32,
        "hidden profile panel rail width"
    );
    require(
        ovtr::win32::rightProfileAreaWidthForClient(true, 1200) == 360,
        "visible profile panel width"
    );
    require(
        ovtr::win32::clampProfilePanelWidthForClient(100, 1200) == 260,
        "profile panel min clamp"
    );
    require(
        ovtr::win32::clampProfilePanelWidthForClient(800, 1200) == 720,
        "profile panel max clamp"
    );

    ovtr::win32::AppWindowState state;
    state.debugMonitorVisible = true;
    state.debugMonitorHeight = 220;
    state.devicePanelVisible = true;
    state.leftPanelWidth = 500;
    ovtr::DeviceDescriptor controller;
    controller.runtimeIndex = 3;
    controller.deviceClass = ovtr::DeviceClass::Controller;
    controller.serial = "CTRL";
    controller.modelName = "Controller";
    state.devices.push_back(controller);
    ovtr::DeviceDescriptor hmd;
    hmd.runtimeIndex = 1;
    hmd.deviceClass = ovtr::DeviceClass::Hmd;
    hmd.serial = "HMD";
    hmd.modelName = "Headset";
    state.devices.push_back(hmd);
    require(ovtr::win32::activeDebugMonitorHeight(&state, 800) == 220, "state debug monitor height");
    require(ovtr::win32::leftPanelContentBottomForClient(&state, 800) == 544, "state content bottom");
    require(ovtr::win32::leftPanelWidthForClient(&state, 1200) == 500, "state left panel width");
    require(
        sameRect(ovtr::win32::splitterRectForClient(&state, 1200, 800), 500, 32, 508, 544),
        "state splitter rect"
    );
    require(
        sameRect(ovtr::win32::deviceToggleButtonRectForClient(&state, 1200, 800), 4, 44, 28, 140),
        "state device toggle rect"
    );
    require(!state.profilePanelVisible, "profile panel defaults hidden");
    require(
        sameRect(ovtr::win32::profileToggleButtonRectForClient(&state, 1200, 800), 1172, 44, 1196, 156),
        "state profile toggle rect"
    );
    require(
        sameRect(ovtr::win32::mappingToggleButtonRectForClient(&state, 1200, 800), 1172, 164, 1196, 276),
        "state mapping toggle rect"
    );
    require(
        !ovtr::win32::profilePanelLayoutForClient(&state, 1200, 800).valid,
        "state profile panel hidden by default"
    );
    state.profilePanelVisible = true;
    require(
        sameRect(ovtr::win32::profilePanelLayoutForClient(&state, 1200, 800).panelRect, 848, 32, 1168, 544),
        "state profile panel rect"
    );
    require(
        sameRect(ovtr::win32::profileSplitterRectForClient(&state, 1200, 800), 840, 32, 848, 544),
        "state profile splitter rect"
    );
    require(
        sameRect(ovtr::win32::viewportRenderRectForClient(&state, 1200, 800), 508, 32, 840, 496),
        "state viewport render rect reserves profile panel"
    );
    state.profilePanelWidth = 400;
    require(
        ovtr::win32::rightProfileAreaWidthForClient(&state, 1200) == 440,
        "state profile width uses requested width"
    );
    state.profilePanelVisible = false;
    state.mappingPanelVisible = true;
    require(
        sameRect(ovtr::win32::profilePanelLayoutForClient(&state, 1200, 800).panelRect, 768, 32, 1168, 544),
        "state mapping panel uses requested width"
    );
    state.mappingPanelVisible = false;
    const ovtr::win32::DeviceListLayout stateDeviceList =
        ovtr::win32::deviceListLayoutForClient(&state, 1200, 800);
    require(stateDeviceList.valid, "state device list layout is valid");
    require(
        ovtr::win32::deviceRuntimeIndexFromListPoint(state, stateDeviceList, POINT{70, 90}) == 1,
        "state device hit test uses sorted rows"
    );
    state.deviceListScrollOffset = 99;
    ovtr::win32::clampDeviceListScroll(state, stateDeviceList.visibleItemCount);
    require(state.deviceListScrollOffset == 0, "state device scroll clamps");
    state.debugLogLines = {L"1", L"2", L"3", L"4", L"5"};
    state.debugLogScrollOffset = 99;
    ovtr::win32::clampDebugLogScroll(state, 2);
    require(state.debugLogScrollOffset == 3, "state debug log scroll clamps");

    require(ovtr::win32::contentBottomForClient(0, 800) == 764, "content bottom without debug monitor");
    require(ovtr::win32::contentBottomForClient(220, 800) == 544, "content bottom with debug monitor");
    require(
        sameRect(ovtr::win32::splitterRectForClient(420, 220, 800), 420, 32, 428, 544),
        "splitter rect"
    );
    require(
        sameRect(ovtr::win32::deviceToggleButtonRectForClient(764, 1200, 800), 4, 44, 28, 140),
        "device toggle button rect"
    );
    require(
        sameRect(ovtr::win32::profileToggleButtonRectForClient(764, 1200, 800), 1172, 44, 1196, 156),
        "profile toggle button rect"
    );
    require(
        sameRect(ovtr::win32::mappingToggleButtonRectForClient(764, 1200, 800), 1172, 164, 1196, 276),
        "mapping toggle button rect"
    );
    require(
        sameRect(ovtr::win32::deviceToggleButtonRectForClient(80, 1200, 800), 0, 0, 0, 0),
        "device toggle rejects short content"
    );
}

} // namespace ovtr::test
