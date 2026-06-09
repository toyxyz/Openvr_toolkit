#include "TestCases.h"
#include "TestSupport.h"
#include "Win32LayoutTestSupport.h"

#include "platform/win32/AppProfileState.h"
#include "platform/win32/AppState.h"
#include "platform/win32/MappingActorActions.h"
#include "platform/win32/MappingActions.h"
#include "platform/win32/MappingActorLayout.h"
#include "platform/win32/MappingModel.h"
#include "platform/win32/MappingPanelLayout.h"
#include "platform/win32/MappingPresetStore.h"
#include "platform/win32/WindowLayout.h"

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace ovtr::test {

void testWin32MappingModel()
{
    const auto& definitions = ovtr::win32::mappingSlotDefinitions();
    require(definitions.size() == ovtr::win32::kMappingSlotCount, "mapping slot count");
    require(std::wstring(ovtr::win32::kMappingNoDeviceLabel) == L"None", "mapping none label");
    require(std::wstring(definitions[0].label) == L"Head", "mapping first label");
    require(std::wstring(definitions[10].label) == L"Right Foot", "mapping last label");

    ovtr::win32::AppProfileState state{};
    require(!state.mappingPanelVisible, "mapping panel defaults hidden");
    require(state.mappingActors.empty(), "mapping actors default empty");
    require(state.nextMappingActorId == 1, "mapping actor id default");
    require(state.selectedMappingActorId == 0, "mapping actor selection default");
    require(state.mappingActorScrollOffset == 0, "mapping actor scroll default");
    require(state.mappingDropdownSlot == -1, "mapping dropdown defaults closed");
    require(state.mappingFingerRuntimeIndices[0] == ovtr::win32::kNoSelectedRuntimeIndex, "left finger defaults none");
    require(state.mappingFingerRuntimeIndices[1] == ovtr::win32::kNoSelectedRuntimeIndex, "right finger defaults none");
    require(!state.mappingProfileDropdownOpen, "mapping profile dropdown defaults closed");
    require(!state.mappingPresetDropdownOpen, "mapping preset dropdown defaults closed");
    require(state.mappingArmSoftIkStrength == ovtr::win32::kDefaultMappingArmSoftIkStrength, "arm soft IK default");
    require(state.mappingLegSoftIkStrength == ovtr::win32::kDefaultMappingLegSoftIkStrength, "leg soft IK default");
    require(ovtr::win32::kMappingSoftIkStrengthOptions.size() == 7, "soft IK option count");
    require(ovtr::win32::kMappingSoftIkStrengthOptions[1] == 0.01f, "soft IK 1 percent option");
    for (const std::uint32_t runtimeIndex : state.mappingDeviceRuntimeIndices) {
        require(runtimeIndex == ovtr::win32::kNoSelectedRuntimeIndex, "mapping default device is empty");
    }

    ovtr::win32::MappingPreset preset;
    preset.name = L"actor_01";
    preset.actorName = L"Hero Actor";
    preset.hasProfile = true;
    preset.profile.name = L"profile_in_mapping";
    preset.profile.measurements[0] = 92.0f;
    preset.profile.measurements[4] = 9.0f;
    preset.profile.measurements[13] = 4.0f;
    preset.skeletonColor = ovtr::win32::RgbColor{12, 34, 56};
    preset.deviceSerials[0] = L"hmd_serial";
    preset.deviceSerials[10] = L"foot_serial";
    preset.fingerSerials[0] = L"Left hand";
    preset.fingerSerials[1] = L"Right hand";
    std::istringstream input(ovtr::win32::serializeMappingPreset(preset));
    ovtr::win32::MappingPreset parsed;
    std::string error;
    require(ovtr::win32::parseMappingPreset(input, parsed, error), "mapping preset parses");
    require(parsed.name == L"actor_01", "mapping preset name round trip");
    require(parsed.actorName == L"Hero Actor", "mapping preset actor name round trip");
    require(parsed.hasProfile, "mapping preset includes profile");
    require(parsed.profile.name == L"profile_in_mapping", "mapping preset profile name round trip");
    require(parsed.profile.measurements[0] == 92.0f, "mapping preset profile pelvis round trip");
    require(parsed.profile.measurements[4] == 9.0f, "mapping preset profile neck round trip");
    require(parsed.profile.measurements[13] == 4.0f, "mapping preset profile toe round trip");
    require(parsed.skeletonColor.r == 12, "mapping preset color r");
    require(parsed.skeletonColor.g == 34, "mapping preset color g");
    require(parsed.skeletonColor.b == 56, "mapping preset color b");
    require(parsed.deviceSerials[0] == L"hmd_serial", "mapping preset first serial");
    require(parsed.deviceSerials[10] == L"foot_serial", "mapping preset last serial");
    require(parsed.fingerSerials[0] == L"Left hand", "mapping preset left finger serial");
    require(parsed.fingerSerials[1] == L"Right hand", "mapping preset right finger serial");

    std::istringstream legacyPreset(
        "version=2\nname=legacy\narm_mode=fk\nslot_0=\nslot_1=\nslot_2=\nslot_3=\nslot_4=\nslot_5=\n"
        "slot_6=\nslot_7=\nslot_8=\nslot_9=\nslot_10=\n"
    );
    require(ovtr::win32::parseMappingPreset(legacyPreset, parsed, error), "old arm_mode mapping preset parses");
    require(!parsed.hasProfile, "legacy mapping preset has no embedded profile");
    require(parsed.actorName == L"legacy", "legacy mapping preset actor name fallback");
    require(parsed.skeletonColor.r == 255, "legacy mapping preset default color");
    require(parsed.fingerSerials[0].empty(), "legacy mapping preset left finger defaults none");
    require(parsed.fingerSerials[1].empty(), "legacy mapping preset right finger defaults none");

    ovtr::win32::MappingActor actor;
    actor.id = 77;
    actor.calibrated = true;
    state.mappingActors.push_back(actor);
    state.mappingArmSoftIkStrength = 0.0f;
    state.mappingLegSoftIkStrength = 0.15f;
    ovtr::win32::syncMappingSoftIkStrengthsToActors(state);
    require(state.mappingActors[0].calibration.armSoftIkStrength == 0.0f, "arm soft IK live sync");
    require(state.mappingActors[0].calibration.legSoftIkStrength == 0.15f, "leg soft IK live sync");
    require(ovtr::win32::toggleMappingActorSelectionAtIndex(state, 0), "mapping actor selects");
    require(state.selectedMappingActorId == 77, "mapping actor selected id");
    require(ovtr::win32::toggleMappingActorSelectionAtIndex(state, 0), "mapping actor re-click toggles");
    require(state.selectedMappingActorId == 0, "mapping actor re-click clears selection");

    ovtr::win32::AppProfileState selectionState;
    ovtr::win32::MappingActor selectionActor;
    selectionActor.id = 11;
    selectionActor.name = L"custom_actor";
    selectionActor.profile.name = L"actor_profile";
    selectionActor.skeletonColor = ovtr::win32::RgbColor{1, 2, 3};
    selectionActor.mappingDeviceRuntimeIndices[0] = 123;
    selectionActor.mappingFingerRuntimeIndices[0] = 321;
    selectionActor.mappingFingerRuntimeIndices[1] = 322;
    selectionState.mappingActors.push_back(selectionActor);
    selectionState.profile.name = L"top_profile";
    require(ovtr::win32::toggleMappingActorSelectionAtIndex(selectionState, 0), "actor selection syncs");
    require(selectionState.mappingActorName == L"custom_actor", "selected actor name reaches controls");
    require(selectionState.profile.name == L"actor_profile", "selected actor profile reaches controls");
    require(selectionState.mappingSkeletonColor.g == 2, "selected actor color reaches controls");
    require(selectionState.mappingDeviceRuntimeIndices[0] == 123, "selected actor mapping reaches controls");
    require(selectionState.mappingFingerRuntimeIndices[0] == 321, "selected actor left finger reaches controls");
    require(selectionState.mappingFingerRuntimeIndices[1] == 322, "selected actor right finger reaches controls");
    selectionState.profile.name = L"edited_profile";
    selectionState.mappingActorName = L"renamed_actor";
    selectionState.mappingSkeletonColor = ovtr::win32::RgbColor{4, 5, 6};
    selectionState.mappingDeviceRuntimeIndices[0] = 456;
    selectionState.mappingFingerRuntimeIndices[0] = 654;
    selectionState.mappingFingerRuntimeIndices[1] = 655;
    selectionState.mappingActors[0].calibrated = true;
    ovtr::win32::syncSelectedMappingActorFromControls(selectionState);
    require(selectionState.mappingActors[0].name == L"renamed_actor", "controls actor name updates actor");
    require(selectionState.mappingActors[0].profile.name == L"edited_profile", "controls profile updates actor");
    require(selectionState.mappingActors[0].skeletonColor.b == 6, "controls color updates actor");
    require(selectionState.mappingActors[0].mappingDeviceRuntimeIndices[0] == 456, "controls mapping updates actor");
    require(selectionState.mappingActors[0].mappingFingerRuntimeIndices[0] == 654, "controls left finger updates actor");
    require(selectionState.mappingActors[0].mappingFingerRuntimeIndices[1] == 655, "controls right finger updates actor");
    require(selectionState.mappingActors[0].calibrated, "source change keeps actor calibration");
}

void testWin32MappingPanelLayout()
{
    ovtr::win32::ProfilePanelLayout panel;
    panel.panelRect = RECT{848, 32, 1168, 544};
    panel.valid = true;

    const ovtr::win32::MappingPanelControlsLayout controls =
        ovtr::win32::mappingControlsLayoutForPanel(panel);
    require(controls.valid, "mapping panel controls valid");
    require(sameRect(controls.profileBoxRect, 858, 42, 1158, 70), "mapping profile box");
    require(sameRect(controls.tableRect, 858, 78, 1158, 162), "mapping list box below profile");
    require(sameRect(controls.colorBoxRect, 858, 166, 1158, 194), "mapping color box below list");
    require(sameRect(controls.actorNameBoxRect, 858, 198, 1158, 226), "mapping actor name box below color");
    require(sameRect(controls.nameBoxRect, 858, 230, 1158, 258), "mapping preset name box below actor name");
    require(sameRect(controls.presetSaveButtonRect, 858, 262, 944, 290), "mapping preset save button");
    require(sameRect(controls.presetValueRect, 952, 262, 1158, 290), "mapping preset dropdown");
    require(sameRect(controls.addActorButtonRect, 858, 296, 1158, 324), "mapping add actor button");
    require(sameRect(controls.actorListRect, 858, 332, 1158, 416), "mapping actor list");
    require(sameRect(controls.calibrateButtonRect, 858, 422, 1158, 450), "mapping calibrate button");
    require(sameRect(controls.filterBoxRect, 858, 458, 1158, 526), "mapping filter box");
    require(controls.visibleRowCount == 3, "mapping full visible row count");
    require(ovtr::win32::maxMappingScrollOffset(controls.visibleRowCount) == 10, "mapping full scroll");

    const std::vector<ovtr::win32::MappingPanelRowLayout> rows =
        ovtr::win32::mappingRowLayoutsForPanel(panel);
    require(rows.size() == 3, "mapping row count");
    require(rows.front().slotIndex == 0, "mapping first row is first device slot");
    require(rows.back().slotIndex == 2, "mapping last visible row slot");
    const std::vector<ovtr::win32::MappingActorRowLayout> actorRows =
        ovtr::win32::mappingActorRowLayouts(controls, 4, 1);
    require(actorRows.size() == 3, "mapping actor row count");
    require(actorRows.front().actorIndex == 1, "mapping scrolled first actor row");
    require(ovtr::win32::maxMappingActorScrollOffset(4, 3) == 1, "mapping actor max scroll");

    panel.panelRect = RECT{848, 32, 1168, 390};
    const ovtr::win32::MappingPanelControlsLayout smallControls =
        ovtr::win32::mappingControlsLayoutForPanel(panel);
    require(smallControls.valid, "small mapping panel controls valid");
    require(smallControls.visibleRowCount == 1, "small mapping visible row count");
    require(ovtr::win32::maxMappingScrollOffset(smallControls.visibleRowCount) == 12, "mapping max scroll");
    require(
        ovtr::win32::clampMappingScrollOffset(99, smallControls.visibleRowCount) == 12,
        "mapping scroll clamps high"
    );

    const std::vector<ovtr::win32::MappingPanelRowLayout> scrolledRows =
        ovtr::win32::mappingRowLayoutsForPanel(panel, 4);
    require(scrolledRows.size() == 1, "mapping scrolled row count");
    require(scrolledRows.front().slotIndex == 4, "mapping scroll first visible slot");
    const POINT hitPoint{scrolledRows.front().valueRect.left + 2, scrolledRows.front().valueRect.top + 2};
    const ovtr::win32::MappingPanelRowLayout hitRow =
        ovtr::win32::mappingRowLayoutAtPoint(panel, hitPoint, 4);
    require(hitRow.slotIndex == 4, "mapping scrolled hit target");
    require(std::wstring(ovtr::win32::mappingPanelRowLabel(10)) == L"Right Foot", "mapping last row label");
    require(std::wstring(ovtr::win32::mappingPanelRowLabel(11)) == L"Left Finger", "mapping left finger row label");
    require(std::wstring(ovtr::win32::mappingPanelRowLabel(12)) == L"Right Finger", "mapping right finger row label");

    ovtr::win32::AppProfileState defaultNameState;
    defaultNameState.profile.name = L"profile_name";
    defaultNameState.mappingActorName = L"manual_name";
    ovtr::win32::addMappingActorFromProfile(defaultNameState, defaultNameState.profile, 3);
    ovtr::win32::addMappingActorFromProfile(defaultNameState, defaultNameState.profile, 3);
    ovtr::win32::addMappingActorFromProfile(defaultNameState, defaultNameState.profile, 3);
    require(defaultNameState.mappingActors[0].name == L"actor_0", "first actor gets default suffix");
    require(defaultNameState.mappingActors[1].name == L"actor_1", "second actor increments default suffix");
    require(defaultNameState.mappingActors[2].name == L"actor_2", "third actor increments default suffix");
    require(defaultNameState.mappingActorName == L"actor_2", "actor name editor reflects default name");

    const RECT dropdown = ovtr::win32::mappingDropdownRectForRow(scrolledRows.front(), panel, 2);
    require(dropdown.right > dropdown.left && dropdown.bottom > dropdown.top, "mapping dropdown rect valid");
    require(
        ovtr::win32::mappingDropdownOptionFromPoint(
            scrolledRows.front(),
            panel,
            2,
            POINT{dropdown.left + 2, dropdown.top + 2}
        ) == 0,
        "mapping dropdown option hit"
    );
    const RECT profileDropdown =
        ovtr::win32::mappingProfileDropdownRectForControls(smallControls, panel, 2);
    require(profileDropdown.right > profileDropdown.left, "mapping profile dropdown rect valid");
    require(
        ovtr::win32::mappingProfileDropdownOptionFromPoint(
            smallControls,
            panel,
            2,
            POINT{profileDropdown.left + 2, profileDropdown.top + 2}
        ) == 0,
        "mapping profile dropdown option hit"
    );
    const RECT presetDropdown =
        ovtr::win32::mappingPresetDropdownRectForControls(smallControls, panel, 2);
    require(presetDropdown.right > presetDropdown.left, "mapping preset dropdown rect valid");
    require(
        ovtr::win32::mappingPresetDropdownOptionFromPoint(
            smallControls,
            panel,
            2,
            POINT{presetDropdown.left + 2, presetDropdown.top + 2}
        ) == 0,
        "mapping preset dropdown option hit"
    );

    ovtr::win32::AppProfileState state;
    for (int index = 0; index < 4; ++index) {
        ovtr::win32::addMappingActorFromProfile(state, state.profile, 3);
        const std::wstring expectedName = L"actor_" + std::to_wstring(index);
        require(state.mappingActors.back().name == expectedName, "new mapping actor gets default name");
        require(
            state.selectedMappingActorId == state.mappingActors.back().id,
            "new mapping actor is selected"
        );
    }
    require(state.mappingActorScrollOffset == 1, "new mapping actor is scrolled into view");
}

void testWin32MappingPanelStateLayout()
{
    ovtr::win32::AppWindowState state;
    state.debugMonitorVisible = true;
    state.debugMonitorHeight = 220;
    state.devicePanelVisible = true;
    state.leftPanelWidth = 500;

    require(
        sameRect(ovtr::win32::mappingToggleButtonRectForClient(&state, 1200, 800), 1172, 164, 1196, 276),
        "state mapping toggle rect"
    );
    state.mappingPanelVisible = true;
    const ovtr::win32::ProfilePanelLayout panel = ovtr::win32::profilePanelLayoutForClient(&state, 1200, 800);
    require(
        sameRect(panel.panelRect, 848, 32, 1168, 544),
        "state mapping panel rect"
    );
    require(
        ovtr::win32::rightProfileAreaWidthForClient(&state, 1200) == 360,
        "state mapping reserves right panel width"
    );
}

} // namespace ovtr::test
