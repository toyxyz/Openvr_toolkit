#include "TestCases.h"
#include "TestSupport.h"
#include "Win32LayoutTestSupport.h"

#include "platform/win32/AppState.h"
#include "platform/win32/Layout.h"
#include "platform/win32/MappingEditPanelLayout.h"
#include "platform/win32/RecordingSessionList.h"
#include "platform/win32/WindowLayout.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

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
    require(
        sameRect(ovtr::win32::sessionToggleButtonRectForClient(&state, 1200, 800), 4, 148, 28, 260),
        "state session toggle rect"
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
        sameRect(ovtr::win32::editToggleButtonRectForClient(&state, 1200, 800), 1172, 284, 1196, 348),
        "state edit toggle rect"
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
    state.editPanelVisible = true;
    const ovtr::win32::ProfilePanelLayout editPanel =
        ovtr::win32::profilePanelLayoutForClient(&state, 1200, 800);
    require(sameRect(editPanel.panelRect, 768, 32, 1168, 544), "state edit panel uses requested width");
    const ovtr::win32::MappingEditPanelLayout editLayout =
        ovtr::win32::mappingEditPanelLayoutForPanel(editPanel);
    require(editLayout.valid, "mapping edit layout valid");
    require(
        ovtr::win32::mappingEditStepOptionsForPanel(editPanel).size() == 3,
        "mapping edit step dropdown has three options"
    );
    require(state.mappingEditOffsetStepMeters == 0.001f, "mapping edit step defaults to one millimeter");
    state.editPanelVisible = false;
    state.sessionPanelVisible = true;
    state.devicePanelVisible = false;
    const ovtr::win32::SessionListLayout stateSessionList =
        ovtr::win32::sessionListLayoutForClient(&state, 1200, 800, 3);
    require(stateSessionList.valid, "state session list layout is valid");
    require(stateSessionList.boxRect.top == 44, "state session list top anchored");
    require(stateSessionList.boxRect.bottom - stateSessionList.boxRect.top > 180, "state session list extends downward");
    state.sessionListScrollOffset = 99;
    ovtr::win32::clampSessionListScroll(state, 3, stateSessionList.visibleItemCount);
    require(state.sessionListScrollOffset == 0, "state session scroll clamps");
    require(
        ovtr::win32::sessionListRowIndexFromPoint(
            stateSessionList,
            POINT{stateSessionList.contentRect.left + 4, stateSessionList.contentRect.top + 4},
            3,
            0
        ) == 0,
        "state session hit test selects first row"
    );
    state.sessionPanelVisible = false;
    state.devicePanelVisible = true;
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
        sameRect(ovtr::win32::sessionToggleButtonRectForClient(764, 1200, 800), 4, 148, 28, 260),
        "session toggle button rect"
    );
    const ovtr::win32::SessionListLayout tallSessionList =
        ovtr::win32::sessionListLayoutForClient(true, 500, 1100, false, 0, 20);
    require(tallSessionList.boxRect.top == 44, "tall session list is top anchored");
    require(
        tallSessionList.boxRect.bottom - tallSessionList.boxRect.top == 900,
        "session list grows to five-times previous height"
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
        sameRect(ovtr::win32::editToggleButtonRectForClient(764, 1200, 800), 1172, 284, 1196, 348),
        "edit toggle button rect"
    );
    require(
        sameRect(ovtr::win32::deviceToggleButtonRectForClient(80, 1200, 800), 0, 0, 0, 0),
        "device toggle rejects short content"
    );

    const std::filesystem::path sessionRoot =
        std::filesystem::current_path() / ".tmp_ovtr_session_list_order";
    std::error_code cleanupError;
    std::filesystem::remove_all(sessionRoot, cleanupError);
    require(std::filesystem::create_directories(sessionRoot), "session list test root");
    const auto makeSessionFolder = [&](const std::wstring& name) {
            const std::filesystem::path folder = sessionRoot / name;
            require(std::filesystem::create_directories(folder), "session folder created");
            std::ofstream manifest(folder / "manifest.json");
            manifest << "{}";
            manifest.close();
        };
    makeSessionFolder(L"session_2026_06_01_010000");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    makeSessionFolder(L"session_2026_06_01_020000");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    makeSessionFolder(L"Alpha_2026_06_01_030000");
    const std::vector<ovtr::win32::RecordingSessionListRow> sessionRows =
        ovtr::win32::listRecordingSessionFolders(sessionRoot);
    require(sessionRows.size() == 3, "session list includes manifest folders");
    require(sessionRows[0].name == L"Alpha_2026_06_01_030000", "newest session folder is first");
    require(sessionRows[1].name == L"session_2026_06_01_020000", "second newest session folder is second");
    require(sessionRows[2].name == L"session_2026_06_01_010000", "oldest session folder is last");
    std::filesystem::remove_all(sessionRoot, cleanupError);
}

} // namespace ovtr::test
