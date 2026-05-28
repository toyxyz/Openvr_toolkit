#include "platform/win32/ViewportSceneRenderer.h"

#include "math/PoseTransform.h"
#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppOriginState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/AppViewportState.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportRenderModelRenderer.h"

#include <gl/GL.h>

namespace ovtr::win32 {
namespace {

void drawDeviceLabel3D(
    const AppDeviceState& deviceState,
    const ovtr::PoseSample& pose,
    const ovtr::DeviceDescriptor* device,
    const GLuint fontBase
)
{
    if ((pose.flags & ovtr::PoseFlagPoseValid) == 0) {
        return;
    }

    const ovtr::DeviceClass deviceClass = device ? device->deviceClass : ovtr::DeviceClass::Other;
    const float labelLift = deviceClass == ovtr::DeviceClass::TrackingReference ? 0.13f : 0.10f;

    glRasterPos3f(pose.position[0], pose.position[1] + labelLift, pose.position[2]);
    drawLabelText3D(labelForDevice(deviceState, device, pose), fontBase);
}

ovtr::PoseSample displayPoseForState(const AppOriginState& originState, const ovtr::PoseSample& pose)
{
    return ovtr::applyOriginToPose(
        pose,
        originState.originEnabled,
        originState.originOffset,
        originState.originRotationDegrees
    );
}

} // namespace

void drawTrackedDevices3D(
    AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState,
    const AppOriginState& originState,
    AppViewportState& viewportState,
    const int viewportHeight,
    const CameraView& cameraView,
    const float outlineWorldUnitsPerPixel
)
{
    for (const ovtr::PoseSample& pose : runtimeState.poses.poses) {
        const ovtr::PoseSample displayPose = displayPoseForState(originState, pose);
        const ovtr::DeviceDescriptor* device = deviceForRuntimeIndex(runtimeState.devices, displayPose.runtimeIndex);
        const ovtr::DeviceClass deviceClass = device ? device->deviceClass : ovtr::DeviceClass::Other;
        const bool selected = displayPose.runtimeIndex == deviceState.selectedDeviceRuntimeIndex;
        const bool modelDrawn = drawSteamVRRenderModel3D(
            viewportState,
            displayPose,
            device,
            viewportHeight,
            selected,
            cameraView,
            outlineWorldUnitsPerPixel
        );
        drawDeviceMarker3D(displayPose, deviceClass, !modelDrawn, selected);
    }
}

void drawTrackedDeviceLabels3D(
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState,
    const AppOriginState& originState,
    const AppViewportState& viewportState
)
{
    for (const ovtr::PoseSample& pose : runtimeState.poses.poses) {
        const ovtr::PoseSample displayPose = displayPoseForState(originState, pose);
        drawDeviceLabel3D(
            deviceState,
            displayPose,
            deviceForRuntimeIndex(runtimeState.devices, displayPose.runtimeIndex),
            viewportState.glLabelFontBase.get()
        );
    }
}

} // namespace ovtr::win32
