#include "TestCases.h"
#include "TestSupport.h"
#include "Win32LayoutTestSupport.h"

#include "platform/win32/AppProfileState.h"
#include "platform/win32/ProfileEditModel.h"
#include "platform/win32/ProfileModel.h"
#include "platform/win32/ProfilePanelLayout.h"
#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/ProfileStore.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

namespace ovtr::test {
namespace {

void requireNear(const float actual, const float expected, const std::string& context)
{
    require(
        std::fabs(actual - expected) < 0.0005f,
        context + " expected " + std::to_string(expected) + " got " + std::to_string(actual)
    );
}

void requireDefaultProfile(const ovtr::win32::BodyProfile& profile, const std::string& context)
{
    require(profile.name == L"actor_01", context + " profile name");
    requireNear(ovtr::win32::computedProfileHeightCm(profile), 170.0f, context + " computed height");
    const std::array<float, ovtr::win32::kProfileMeasurementCount> expectedMeasurements{
        90.0f, 28.0f, 53.0f, 27.0f, 7.0f, 41.0f, 31.0f,
        26.0f, 19.0f, 42.0f, 40.0f, 25.0f, 8.0f, 2.0f
    };
    for (std::size_t index = 0; index < expectedMeasurements.size(); ++index) {
        require(
            std::fabs(profile.measurements[index] - expectedMeasurements[index]) < 0.0005f,
            context + " profile measurement " + std::to_string(index)
                + " expected " + std::to_string(expectedMeasurements[index])
                + " got " + std::to_string(profile.measurements[index])
        );
    }
}

void requireVecNear(const ovtr::win32::Vec3 actual, const ovtr::win32::Vec3 expected, const std::string& context)
{
    requireNear(actual.x, expected.x, context + " x");
    requireNear(actual.y, expected.y, context + " y");
    requireNear(actual.z, expected.z, context + " z");
}

} // namespace

void testWin32ProfileModel()
{
    ovtr::win32::BodyProfile profile;
    requireDefaultProfile(profile, "default");
    ovtr::win32::AppProfileState profileState;
    requireDefaultProfile(profileState.profile, "app state default");
    require(
        ovtr::win32::profileMeasurementDefinitions()[0].key == std::string("floor_to_pelvis_center_height_cm"),
        "profile first measurement is not total height"
    );
    require(
        ovtr::win32::profileMeasurementDefinitions()[4].key == std::string("neck_length_cm"),
        "profile neck length measurement order"
    );
    require(
        ovtr::win32::profileMeasurementDefinitions()[13].key == std::string("toe_tip_height_cm"),
        "profile measurement order"
    );

    float parsed = 0.0f;
    require(ovtr::win32::parseProfileNumberText(L"181.5", parsed), "profile number parses");
    require(parsed > 181.4f && parsed < 181.6f, "profile number value");
    require(!ovtr::win32::parseProfileNumberText(L"-1", parsed), "profile rejects negative");
    require(!ovtr::win32::parseProfileNumberText(L"170cm", parsed), "profile rejects unit suffix");
    require(
        ovtr::win32::sanitizedProfileFileStem(L" actor:01. ") == L"actor_01",
        "profile filename sanitizes invalid characters"
    );
}

void testWin32ProfileSerialization()
{
    ovtr::win32::BodyProfile profile;
    profile.name = L"actor_test";
    profile.measurements[0] = 91.0f;
    profile.measurements[4] = 8.0f;
    profile.measurements[11] = 24.0f;
    profile.measurements[13] = 3.0f;

    const std::string text = ovtr::win32::serializeProfile(profile);
    require(text.find("total_height_cm") == std::string::npos, "profile does not serialize duplicate total height");
    require(text.find("\nheight_cm=") == std::string::npos, "profile does not serialize display height");
    require(text.find("version=4") != std::string::npos, "profile serializes v4 schema");
    require(text.find("neck_length_cm=8") != std::string::npos, "profile serializes neck length");
    require(text.find("toe_tip_height_cm=3") != std::string::npos, "profile serializes toe height");
    std::istringstream input(text);
    ovtr::win32::BodyProfile loaded;
    std::string error;
    require(ovtr::win32::parseProfile(input, loaded, error), "profile parse round trip: " + error);
    require(loaded.name == L"actor_test", "profile name round trip");
    requireNear(ovtr::win32::computedProfileHeightCm(loaded), 171.0f, "profile computed height round trip");
    require(loaded.measurements[0] == 91.0f, "profile measurement round trip");
    require(loaded.measurements[4] == 8.0f, "profile neck length round trip");
    require(loaded.measurements[11] == 24.0f, "profile final measurement round trip");
    require(loaded.measurements[13] == 3.0f, "profile toe height round trip");

    std::istringstream legacy(
        "name=legacy\nheight_cm=170\n"
        "floor_to_pelvis_center_height_cm=90\npelvis_width_cm=28\n"
        "pelvis_center_to_neck_base_center_cm=53\nneck_base_center_to_head_top_cm=27\n"
        "shoulder_width_cm=41\nshoulder_to_elbow_cm=31\nelbow_to_wrist_cm=26\n"
        "wrist_to_middle_finger_tip_cm=19\nhip_to_knee_cm=42\n"
        "knee_to_ankle_cm=40\nankle_to_toe_tip_cm=25\n"
    );
    require(ovtr::win32::parseProfile(legacy, loaded, error), "legacy profile parse: " + error);
    require(loaded.measurements[4] == 7.0f, "legacy profile default neck length");
    require(loaded.measurements[12] == 8.0f, "legacy profile default ankle height");
    require(loaded.measurements[13] == 2.0f, "legacy profile default toe height");

    std::istringstream invalid("name=actor\nheight_cm=170\n");
    require(!ovtr::win32::parseProfile(invalid, loaded, error), "profile parse rejects missing fields");
}

void testWin32ProfilePanelLayout()
{
    ovtr::win32::ProfilePanelLayout panel;
    panel.panelRect = RECT{848, 32, 1168, 544};
    panel.valid = true;

    const ovtr::win32::ProfilePanelControlsLayout controls =
        ovtr::win32::profileControlsLayoutForPanel(panel);
    require(controls.valid, "profile panel controls valid");
    require(sameRect(controls.tableRect, 858, 42, 1158, 458), "profile list box hugs rows");
    require(sameRect(controls.previewButtonRect, 858, 466, 1158, 494), "profile preview below list");
    require(sameRect(controls.saveButtonRect, 858, 502, 1004, 530), "profile save below preview");
    require(sameRect(controls.loadButtonRect, 1012, 502, 1158, 530), "profile load below preview");

    const std::vector<ovtr::win32::ProfilePanelFieldLayout> fields =
        ovtr::win32::profileFieldLayoutsForPanel(panel);
    require(fields.size() == 16, "profile field row count");
    require(fields.back().rowRect.bottom == controls.tableRect.bottom, "profile rows fill list box");

    panel.panelRect = RECT{848, 32, 1168, 300};
    const ovtr::win32::ProfilePanelControlsLayout smallControls =
        ovtr::win32::profileControlsLayoutForPanel(panel);
    require(smallControls.valid, "small profile panel controls valid");
    require(smallControls.visibleRowCount == 6, "small profile visible row count");
    require(ovtr::win32::maxProfileScrollOffset(smallControls.visibleRowCount) == 10, "profile max scroll");
    require(
        ovtr::win32::clampProfileScrollOffset(99, smallControls.visibleRowCount) == 10,
        "profile scroll clamps high"
    );
    const std::vector<ovtr::win32::ProfilePanelFieldLayout> scrolledFields =
        ovtr::win32::profileFieldLayoutsForPanel(panel, 4);
    require(scrolledFields.size() == 6, "profile scrolled field count");
    require(
        scrolledFields.front().target.kind == ovtr::win32::ProfileEditKind::Measurement &&
            scrolledFields.front().target.measurementIndex == 2,
        "profile scroll first visible target"
    );
    const POINT hitPoint{
        scrolledFields.front().valueRect.left + 2,
        scrolledFields.front().valueRect.top + 2
    };
    const ovtr::win32::ProfilePanelFieldLayout hitField =
        ovtr::win32::profileFieldLayoutAtPoint(panel, hitPoint, 4);
    require(hitField.target.measurementIndex == 2, "profile scrolled hit target");
}

void testWin32ProfileLiveEdit()
{
    ovtr::win32::AppProfileState state;
    require(!state.profilePreviewEnabled, "profile preview defaults off");
    state.profilePreviewEnabled = true;
    ovtr::win32::disableProfilePreview(state);
    require(!state.profilePreviewEnabled, "profile preview disables");

    const ovtr::win32::ProfileEditTarget heightTarget{ovtr::win32::ProfileEditKind::Height, -1};
    require(!ovtr::win32::profileEditTargetIsValid(heightTarget), "height is display-only");

    const ovtr::win32::ProfileEditTarget pelvisTarget{ovtr::win32::ProfileEditKind::Measurement, 0};
    require(ovtr::win32::applyProfileEditLiveText(state.profile, pelvisTarget, L"91"), "live measurement update");
    requireNear(state.profile.measurements[0], 91.0f, "live measurement value");
    requireNear(ovtr::win32::computedProfileHeightCm(state.profile), 171.0f, "live computed height value");
    require(!ovtr::win32::applyProfileEditLiveText(state.profile, pelvisTarget, L"abc"), "live rejects invalid");
    requireNear(state.profile.measurements[0], 91.0f, "invalid live keeps measurement");

    ovtr::win32::beginProfileEditSnapshot(state);
    require(ovtr::win32::applyProfileEditLiveText(state.profile, pelvisTarget, L"92"), "snapshot live update");
    require(ovtr::win32::restoreProfileEditSnapshot(state), "snapshot restores");
    requireNear(ovtr::win32::computedProfileHeightCm(state.profile), 171.0f, "snapshot restore height");

    std::wstring error;
    require(
        !ovtr::win32::applyProfileEditCommittedText(state.profile, pelvisTarget, L"bad", error),
        "committed edit rejects invalid numeric"
    );
    require(error == L"Enter a non-negative numeric value.", "committed numeric error");
}

void testWin32ProfileSkeleton()
{
    const ovtr::win32::ProfileSkeletonJoints joints =
        ovtr::win32::buildProfileSkeletonJoints(ovtr::win32::BodyProfile{});
    require(joints.size() == ovtr::win32::kProfileSkeletonJointCount, "profile skeleton joint count");
    require(joints[0].parentIndex == -1, "hips parent");
    require(joints[1].parentIndex == 0, "spine parent");
    require(joints[6].parentIndex == 5, "head end parent");
    require(joints[8].parentIndex == 7, "left arm parent");
    require(joints[22].parentIndex == 21, "right toe parent");
    require(
        joints[ovtr::win32::kProfileJointLeftHandMiddle1].parentIndex == ovtr::win32::kProfileJointLeftHand,
        "left middle finger root parent"
    );
    require(
        joints[ovtr::win32::kProfileJointLeftHandMiddle4].parentIndex == ovtr::win32::kProfileJointLeftHandMiddle3,
        "left middle finger tip parent"
    );

    requireVecNear(joints[0].positionMeters, {0.0f, 0.9f, 0.0f}, "hips");
    requireVecNear(joints[16].positionMeters, {0.14f, 0.48f, 0.0f}, "left knee");
    requireVecNear(joints[17].positionMeters, {0.14f, 0.08f, 0.0f}, "left ankle");
    requireVecNear(joints[3].positionMeters, {0.0f, 1.43f, 0.0f}, "spine2");
    requireVecNear(joints[4].positionMeters, {0.0f, 1.50f, 0.0f}, "neck");
    requireVecNear(joints[5].positionMeters, {0.0f, 1.60f, 0.0f}, "head");
    requireVecNear(joints[6].positionMeters, {0.0f, 1.70f, 0.0f}, "head top");
    requireVecNear(joints[10].positionMeters, {0.775f, 1.43f, 0.0f}, "left hand root");
    requireVecNear(joints[ovtr::win32::kProfileJointLeftHandIndex1].positionMeters, {0.8206f, 1.43f, 0.0247f}, "left index root");
    requireVecNear(joints[ovtr::win32::kProfileJointLeftHandIndex4].positionMeters, {0.9498f, 1.43f, 0.0247f}, "left index tip");
    requireVecNear(
        joints[ovtr::win32::kProfileJointLeftHandMiddle4].positionMeters,
        {0.965f, 1.43f, 0.0f},
        "left middle fingertip"
    );
    requireVecNear(joints[ovtr::win32::kProfileJointLeftHandRing4].positionMeters, {0.9555f, 1.43f, -0.019f}, "left ring tip");
    requireVecNear(joints[ovtr::win32::kProfileJointLeftHandPinky4].positionMeters, {0.9232f, 1.43f, -0.038f}, "left pinky tip");
    requireVecNear(joints[ovtr::win32::kProfileJointLeftHandThumb4].positionMeters, {0.8985f, 1.43f, 0.0798f}, "left thumb tip");
    requireVecNear(
        joints[ovtr::win32::kProfileJointRightHandMiddle4].positionMeters,
        {-0.965f, 1.43f, 0.0f},
        "right middle fingertip"
    );
    requireVecNear(joints[ovtr::win32::kProfileJointRightHandIndex4].positionMeters, {-0.9498f, 1.43f, 0.0247f}, "right index tip");
    requireVecNear(joints[ovtr::win32::kProfileJointRightHandThumb4].positionMeters, {-0.8985f, 1.43f, 0.0798f}, "right thumb tip");
    requireVecNear(joints[22].positionMeters, {-0.14f, 0.02f, 0.25f}, "right toe");

    ovtr::win32::BodyProfile longNeck;
    longNeck.measurements[4] = 9.0f;
    const ovtr::win32::ProfileSkeletonJoints longNeckJoints =
        ovtr::win32::buildProfileSkeletonJoints(longNeck);
    requireVecNear(longNeckJoints[4].positionMeters, {0.0f, 1.52f, 0.0f}, "custom neck length");
    requireVecNear(longNeckJoints[6].positionMeters, {0.0f, 1.70f, 0.0f}, "custom neck keeps head top");
}

} // namespace ovtr::test
