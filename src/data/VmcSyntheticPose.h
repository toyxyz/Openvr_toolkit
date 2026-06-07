#pragma once

#include "data/SessionTypes.h"
#include "data/SkeletalSyntheticPose.h"

#include <cstdint>
#include <string>

namespace ovtr {

inline constexpr std::uint32_t kVmcFingerBoneCount = 21;
inline constexpr std::uint32_t kVmcFingerRuntimeIndexBase = 0x0E000000u;
inline constexpr float kVmcFingerBoneBoxEdgeMeters = 0.0018f;
inline constexpr const char* kVmcFingerBoxRenderModelName = "ovtr_vmc_finger_box";

std::uint32_t vmcFingerRuntimeIndex(SkeletalHandSide side, std::uint32_t boneIndex);
bool isVmcFingerRuntimeIndex(std::uint32_t runtimeIndex) noexcept;
bool decodeVmcFingerRuntimeIndex(
    std::uint32_t runtimeIndex,
    SkeletalHandSide& outSide,
    std::uint32_t& outBoneIndex
) noexcept;
DeviceDescriptor makeVmcFingerDeviceDescriptor(SkeletalHandSide side, std::uint32_t boneIndex);
std::string vmcFingerBoneName(std::uint32_t boneIndex);
std::string vmcFingerNodeName(SkeletalHandSide side, std::uint32_t boneIndex);
bool vmcFingerParentIndex(std::uint32_t boneIndex, std::uint32_t& outParentIndex) noexcept;

} // namespace ovtr
