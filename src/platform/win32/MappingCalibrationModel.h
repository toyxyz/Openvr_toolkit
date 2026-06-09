#pragma once

#include "platform/win32/MappingModel.h"
#include "platform/win32/ProfileSkeleton.h"

#include <array>
#include <cstdint>
#include <string>

namespace ovtr::win32 {

struct MappingTransform {
    Vec3 position{};
    std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};
};

struct MappingVirtualTarget {
    MappingTrackerRole role = MappingTrackerRole::Head;
    MappingTransform transform;
    MappingTransform trackerTransform;
    bool valid = false;
};

enum class MappingVirtualTargetSource {
    DirectTracker,
    RestFallback,
    ParentedTracker
};

struct MappingVirtualTargetBinding {
    MappingVirtualTargetSource source = MappingVirtualTargetSource::DirectTracker;
    int parentSlot = -1;
};

inline constexpr int kMappingPoleCount = 4;

struct MappingDebugPole {
    Vec3 root{};
    Vec3 end{};
    Vec3 pole{};
    bool valid = false;
    bool fallback = false;
    bool limited = false;
};

struct MappingCalibrationData {
    std::array<std::uint32_t, kMappingSlotCount> runtimeIndices =
        defaultMappingDeviceRuntimeIndices();
    std::array<MappingTransform, kMappingSlotCount> trackerToTarget{};
    std::array<MappingVirtualTargetBinding, kMappingSlotCount> targetBindings{};
    float armSoftIkStrength = kDefaultMappingArmSoftIkStrength;
    float legSoftIkStrength = kDefaultMappingLegSoftIkStrength;
    bool pinHandTargets = true;
    bool pinFootTargets = true;
};

struct MappingCalibrationStatus {
    bool success = false;
    std::wstring message;
};

} // namespace ovtr::win32
