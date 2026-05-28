#include "TestCases.h"
#include "TestSupport.h"
#include "Win32LayoutTestSupport.h"

#include "math/PoseTransform.h"
#include "platform/win32/AppMarkerState.h"
#include "platform/win32/AppOriginState.h"
#include "platform/win32/Layout.h"
#include "platform/win32/MarkerList.h"
#include "platform/win32/MarkerPoseActions.h"

#include <cmath>

namespace ovtr::test {

void testWin32MarkerState()
{
    ovtr::win32::AppMarkerState state;
    ovtr::win32::appendMarker(state, {1.0f, 2.0f, 3.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, 0.10f);
    ovtr::win32::appendMarker(state, {4.0f, 5.0f, 6.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, 0.10f);
    require(state.markers[0].name == "marker_1", "first marker name mismatch");
    require(state.markers[1].name == "marker_2", "second marker name mismatch");
    require(state.selectedMarkerId == 2, "latest marker should be selected");

    require(ovtr::win32::renameMarker(state, 1, "floor_ref"), "marker rename failed");
    require(state.markers[0].name == "floor_ref", "marker rename mismatch");
    require(!ovtr::win32::renameMarker(state, 1, ""), "empty marker rename should be rejected");
    ovtr::win32::toggleMarkerSelection(state, 2);
    require(state.selectedMarkerId == ovtr::win32::kNoSelectedMarkerId, "selected marker click should clear selection");
    ovtr::win32::toggleMarkerSelection(state, 2);
    require(state.selectedMarkerId == 2, "unselected marker click should select marker");
    require(ovtr::win32::deleteMarker(state, 1), "marker delete failed");
    ovtr::win32::appendMarker(state, {7.0f, 8.0f, 9.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, 0.10f);
    require(state.markers.back().name == "marker_3", "deleted marker id should not be reused");
}

void testWin32MarkerPoseCaptureAppliesOrigin()
{
    ovtr::win32::AppMarkerState markers;
    ovtr::win32::AppOriginState origin;
    origin.originEnabled = true;
    origin.originOffset = {1.0f, 0.0f, 1.0f};
    origin.originRotationDegrees = {0.0f, 90.0f, 0.0f};

    ovtr::PoseSample pose;
    pose.runtimeIndex = 7;
    pose.position = {2.0f, 0.0f, 1.0f};
    pose.rotation = ovtr::quaternionFromEulerDegrees({0.0f, 90.0f, 0.0f});
    pose.flags = ovtr::PoseFlagDeviceConnected | ovtr::PoseFlagPoseValid;
    ovtr::PosePollResult poses;
    poses.poses.push_back(pose);

    std::string status;
    require(
        ovtr::win32::addMarkerFromDevicePose(markers, origin, poses, 7, 0.42f, status),
        "marker pose capture failed: " + status
    );
    require(markers.markers.size() == 1, "marker pose capture count mismatch");
    const ovtr::win32::SceneMarker& marker = markers.markers.front();
    require(std::fabs(marker.position[0]) < 0.0001f, "marker origin position x mismatch");
    require(std::fabs(marker.position[2] - 1.0f) < 0.0001f, "marker origin position z mismatch");
    require(std::fabs(marker.rotation[1]) < 0.0001f, "marker origin rotation y mismatch");
    require(std::fabs(marker.rotation[3] - 1.0f) < 0.0001f, "marker origin rotation w mismatch");
    require(std::fabs(marker.sizeMeters - 0.42f) < 0.0001f, "marker configured size mismatch");
}

void testWin32MarkerListLayout()
{
    const ovtr::win32::MarkerListLayout markers =
        ovtr::win32::markerListLayoutForClient(true, 420, 764, 12);
    require(markers.valid, "marker list layout should be valid");
    require(markers.visibleItemCount == 4, "marker list visible row count mismatch");
    require(sameRect(markers.boxRect, 56, 572, 396, 752), "marker list box rect mismatch");
    require(sameRect(markers.headerRect, 68, 584, 384, 612), "marker list header rect mismatch");
    require(sameRect(markers.contentRect, 68, 612, 384, 740), "marker list content rect mismatch");

    const ovtr::win32::DeviceListLayout devices =
        ovtr::win32::deviceListLayoutForClient(true, 420, 764, true, markers.boxRect.top, 20);
    require(devices.valid, "device list should clip above marker list");
    require(devices.boxRect.bottom < markers.boxRect.top, "device list should not overlap marker list");
    require(
        ovtr::win32::markerListRowIndexFromPoint(markers, POINT{70, 614}, 12, 2) == 2,
        "marker hit-test should include scroll offset"
    );
    require(
        ovtr::win32::clampMarkerListScrollOffset(20, 12, markers.visibleItemCount) == 8,
        "marker scroll should clamp to max offset"
    );
}

} // namespace ovtr::test
