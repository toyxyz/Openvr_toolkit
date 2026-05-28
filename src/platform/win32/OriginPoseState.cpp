#include "platform/win32/OriginState.h"

#include "math/PoseTransform.h"
#include "platform/win32/AppOriginState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/DeviceList.h"

#include <mutex>

namespace ovtr::win32 {

const ovtr::PoseSample* poseForRuntimeIndex(
    const ovtr::PosePollResult& poses,
    const std::uint32_t runtimeIndex
) noexcept
{
    for (const ovtr::PoseSample& pose : poses.poses) {
        if (pose.runtimeIndex == runtimeIndex) {
            return &pose;
        }
    }
    return nullptr;
}

bool isPoseValid(const ovtr::PoseSample& pose) noexcept
{
    return (pose.flags & ovtr::PoseFlagPoseValid) != 0;
}

ovtr::PosePollResult applyOriginToPoses(
    ovtr::PosePollResult poses,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees
)
{
    if (!originEnabled) {
        return poses;
    }

    for (ovtr::PoseSample& pose : poses.poses) {
        pose = ovtr::applyOriginToPose(pose, true, originOffset, originRotationDegrees);
    }
    return poses;
}

bool setOriginFromDevicePose(
    const AppRuntimeState& runtimeState,
    AppOriginState& originState,
    const ovtr::DeviceDescriptor& selected
)
{
    const ovtr::PoseSample* pose = poseForRuntimeIndex(runtimeState.poses, selected.runtimeIndex);
    if (pose == nullptr || !isPoseValid(*pose)) {
        originState.originStatusMessage = "selected device has no valid pose";
        return false;
    }

    std::lock_guard<std::mutex> lock(originState.originMutex);
    originState.originEnabled = true;
    originState.originOffset = pose->position;
    originState.originRotationDegrees = {0.0f, ovtr::yawDegreesFromQuaternion(pose->rotation), 0.0f};
    originState.selectedOriginRuntimeIndex = selected.runtimeIndex;
    originState.originStatusMessage = "origin position and Y rotation set from " + deviceDisplayName(selected);
    return true;
}

} // namespace ovtr::win32
