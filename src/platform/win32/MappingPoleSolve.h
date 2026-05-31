#pragma once

#include "platform/win32/MappingCalibrationModel.h"

namespace ovtr::win32 {

enum class MappingPoleKind {
    LeftArm,
    RightArm,
    LeftLeg,
    RightLeg,
};

int mappingPoleIndex(MappingPoleKind kind) noexcept;

struct MappingPoleSolveInput {
    Vec3 root{};
    Vec3 target{};
    Vec3 hint{};
    Vec3 restMid{};
    Vec3 previousDirection{};
    bool previousDirectionValid = false;
    float upperLength = 0.0f;
    float lowerLength = 0.0f;
};

struct MappingPoleSolveResult {
    Vec3 polePoint{};
    Vec3 direction{};
    bool fallback = false;
    bool limited = false;
    MappingDebugPole debug;
};

MappingPoleSolveResult solveMappingPole(const MappingPoleSolveInput& input) noexcept;

} // namespace ovtr::win32
