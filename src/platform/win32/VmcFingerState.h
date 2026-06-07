#pragma once

#include "data/SkeletalSyntheticPose.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <string>

namespace ovtr::win32 {

inline constexpr int kVmcFingerNameBoneCount = 15;
inline constexpr double kVmcFingerFreshSeconds = 3.0;

struct VmcFingerSideState {
    std::array<std::array<float, 3>, kVmcFingerNameBoneCount> positions{};
    std::array<std::array<float, 4>, kVmcFingerNameBoneCount> rotations{};
    std::array<bool, kVmcFingerNameBoneCount> valid{};
    std::chrono::steady_clock::time_point lastUpdate{};
};

struct VmcFingerSnapshot {
    std::array<VmcFingerSideState, 2> hands{};
};

int vmcSideIndex(ovtr::SkeletalHandSide side) noexcept;
bool isVmcFingerSideFresh(const VmcFingerSideState& state, std::chrono::steady_clock::time_point now) noexcept;
bool parseVmcFingerBoneName(
    const std::string& name,
    ovtr::SkeletalHandSide& outSide,
    std::uint32_t& outBoneIndex
) noexcept;
std::uint32_t vmcPoseBoneIndexForNameBone(std::uint32_t nameBoneIndex) noexcept;

} // namespace ovtr::win32
