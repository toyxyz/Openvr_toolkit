#include "TestCases.h"
#include "TestSupport.h"

#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppOriginState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/OriginState.h"

namespace ovtr::test {

void testWin32OriginState()
{
    ovtr::win32::AppRuntimeState runtimeState;
    ovtr::win32::AppDeviceState deviceState;
    ovtr::win32::AppOriginState originState;

    ovtr::PosePollResult poses;
    ovtr::PoseSample firstPose;
    firstPose.runtimeIndex = 7;
    firstPose.flags = ovtr::PoseFlagPoseValid;
    firstPose.position = {1.0f, 2.0f, 3.0f};
    firstPose.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    poses.poses.push_back(firstPose);

    ovtr::PoseSample secondPose;
    secondPose.runtimeIndex = 8;
    secondPose.flags = 0;
    secondPose.position = {4.0f, 5.0f, 6.0f};
    secondPose.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    poses.poses.push_back(secondPose);

    require(ovtr::win32::poseForRuntimeIndex(poses, 7) == &poses.poses[0], "pose runtime index lookup");
    require(ovtr::win32::poseForRuntimeIndex(poses, 99) == nullptr, "missing pose runtime index lookup");
    require(ovtr::win32::isPoseValid(poses.poses[0]), "valid pose flag check");
    require(!ovtr::win32::isPoseValid(poses.poses[1]), "invalid pose flag check");

    const ovtr::PosePollResult unchanged =
        ovtr::win32::applyOriginToPoses(poses, false, {10.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
    require(unchanged.poses[0].position == firstPose.position, "disabled origin leaves poses unchanged");

    const ovtr::PosePollResult shifted =
        ovtr::win32::applyOriginToPoses(poses, true, {1.0f, 2.0f, 3.0f}, {0.0f, 0.0f, 0.0f});
    require(shifted.poses[0].position == std::array<float, 3>{0.0f, 0.0f, 0.0f}, "enabled origin offsets first pose");
    require(shifted.poses[1].position == std::array<float, 3>{3.0f, 3.0f, 3.0f}, "enabled origin offsets all poses");

    ovtr::DeviceDescriptor controller;
    controller.runtimeIndex = 7;
    controller.deviceClass = ovtr::DeviceClass::Controller;
    controller.serial = "CTRL";
    ovtr::DeviceDescriptor tracker = makeTestTracker("TRACKER");
    tracker.runtimeIndex = 8;
    runtimeState.devices = {controller, tracker};

    deviceState.selectedDeviceRuntimeIndex = 99;
    require(
        ovtr::win32::clearMissingDeviceSelection(runtimeState, deviceState),
        "missing selected device is cleared"
    );
    require(
        deviceState.selectedDeviceRuntimeIndex == ovtr::win32::kNoSelectedRuntimeIndex,
        "missing selected device clears index"
    );
    require(
        ovtr::win32::toggleListDeviceSelectionState(deviceState, controller) ==
            ovtr::win32::ListDeviceSelectionChange::Selected,
        "device selection state selects device"
    );
    require(deviceState.selectedDeviceRuntimeIndex == controller.runtimeIndex, "device selection stores runtime index");
    require(
        !ovtr::win32::clearMissingDeviceSelection(runtimeState, deviceState),
        "available selected device is preserved"
    );
    require(
        ovtr::win32::toggleListDeviceSelectionState(deviceState, controller) ==
            ovtr::win32::ListDeviceSelectionChange::Cleared,
        "device selection state clears selected device"
    );

    originState.selectedOriginRuntimeIndex = ovtr::win32::kNoSelectedRuntimeIndex;
    ovtr::win32::ensureOriginSelection(runtimeState, originState);
    require(
        originState.selectedOriginRuntimeIndex == controller.runtimeIndex,
        "origin selection defaults to first device"
    );
    require(
        ovtr::win32::selectNextOriginDevice(runtimeState, originState) == "selected #8 GenericTracker TRACKER",
        "origin selection advances to next device"
    );
    require(originState.selectedOriginRuntimeIndex == tracker.runtimeIndex, "origin selection stores next device");

    runtimeState.poses = poses;
    require(
        !ovtr::win32::setOriginFromDevicePose(runtimeState, originState, tracker),
        "origin rejects invalid selected pose"
    );
    require(originState.originStatusMessage == "selected device has no valid pose", "origin invalid pose status");
    require(
        ovtr::win32::setOriginFromDevicePose(runtimeState, originState, controller),
        "origin accepts valid selected pose"
    );
    require(originState.originEnabled, "origin from pose enables origin");
    require(originState.originOffset == firstPose.position, "origin from pose stores position");
    require(
        originState.selectedOriginRuntimeIndex == controller.runtimeIndex,
        "origin from pose stores selected runtime index"
    );

    require(ovtr::win32::adjustOriginAxis(originState, false, 0, 0.25f), "origin axis adjustment succeeds");
    require(originState.originOffset[0] == 1.25f, "origin axis adjustment updates offset");
    require(
        originState.selectedOriginRuntimeIndex == ovtr::win32::kNoSelectedRuntimeIndex,
        "origin axis adjustment clears device selection"
    );
    require(originState.originStatusMessage == "origin position X adjusted to 1.250", "origin axis adjustment status");
    require(!ovtr::win32::adjustOriginAxis(originState, true, 3, 0.1f), "origin axis adjustment rejects invalid axis");

}

} // namespace ovtr::test
