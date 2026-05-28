#include "platform/win32/MarkerPoseActions.h"

#include "math/PoseTransform.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/MarkerList.h"
#include "platform/win32/OriginState.h"

#include <array>
#include <mutex>

namespace ovtr::win32 {
namespace {

ovtr::PoseSample displayPoseForMarker(const AppOriginState& state, ovtr::PoseSample pose)
{
    bool originEnabled = false;
    std::array<float, 3> originOffset{};
    std::array<float, 3> originRotationDegrees{};
    {
        std::lock_guard<std::mutex> lock(state.originMutex);
        originEnabled = state.originEnabled;
        originOffset = state.originOffset;
        originRotationDegrees = state.originRotationDegrees;
    }
    return ovtr::applyOriginToPose(pose, originEnabled, originOffset, originRotationDegrees);
}

} // namespace

bool addMarkerFromDevicePose(
    AppMarkerState& markerState,
    const AppOriginState& originState,
    const ovtr::PosePollResult& poses,
    const std::uint32_t runtimeIndex,
    const float markerSizeMeters,
    std::string& statusMessage
)
{
    const ovtr::PoseSample* pose = poseForRuntimeIndex(poses, runtimeIndex);
    if (!pose || !isPoseValid(*pose)) {
        statusMessage = "selected device has no valid pose";
        return false;
    }

    const ovtr::PoseSample displayPose = displayPoseForMarker(originState, *pose);
    ViewportSettings settings;
    settings.markerSize = markerSizeMeters;
    appendMarker(
        markerState,
        displayPose.position,
        displayPose.rotation,
        clampViewportSettings(settings).markerSize
    );
    statusMessage.clear();
    return true;
}

} // namespace ovtr::win32
