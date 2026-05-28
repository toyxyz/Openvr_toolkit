#include "math/PoseTransform.h"

#include "math/QuaternionUtils.h"

namespace ovtr {

PoseSample applyOriginToPose(
    PoseSample pose,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees
)
{
    if (originEnabled) {
        const std::array<float, 4> inverseOriginRotation =
            conjugateQuaternion(quaternionFromEulerDegrees(originRotationDegrees));
        const std::array<float, 3> translatedPosition{
            pose.position[0] - originOffset[0],
            pose.position[1] - originOffset[1],
            pose.position[2] - originOffset[2],
        };
        pose.position = rotatePositionByQuaternion(inverseOriginRotation, translatedPosition);
        pose.rotation = normalizeQuaternion(multiplyQuaternion(inverseOriginRotation, pose.rotation));
    }
    return pose;
}

} // namespace ovtr
