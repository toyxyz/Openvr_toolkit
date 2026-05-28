#pragma once

#include "data/SessionTypes.h"

#include <array>

namespace ovtr {

std::array<float, 4> multiplyQuaternion(
    const std::array<float, 4>& left,
    const std::array<float, 4>& right
);
std::array<float, 4> conjugateQuaternion(const std::array<float, 4>& quaternion);
std::array<float, 4> quaternionFromEulerDegrees(const std::array<float, 3>& degrees);
std::array<float, 3> rotatePositionByQuaternion(
    const std::array<float, 4>& quaternion,
    const std::array<float, 3>& position
);
float yawDegreesFromQuaternion(const std::array<float, 4>& quaternion);
PoseSample applyOriginToPose(
    PoseSample pose,
    bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees
);

} // namespace ovtr
