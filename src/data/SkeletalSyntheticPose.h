#pragma once

#include "data/SessionTypes.h"

#include <cstdint>
#include <string>

namespace ovtr {

enum class SkeletalHandSide {
    Left,
    Right,
};

inline constexpr std::uint32_t kSkeletalHandBoneCount = 31;
inline constexpr std::uint32_t kSkeletalSyntheticRuntimeIndexBase = 0x0F000000u;
inline constexpr float kSkeletalBoneBoxEdgeMeters = 0.009f;
inline constexpr const char* kSkeletalBoneBoxRenderModelName = "ovtr_skeletal_bone_box";

std::uint32_t skeletalBoneRuntimeIndex(SkeletalHandSide side, std::uint32_t boneIndex);
bool isSkeletalBoneRuntimeIndex(std::uint32_t runtimeIndex) noexcept;
bool decodeSkeletalBoneRuntimeIndex(
    std::uint32_t runtimeIndex,
    SkeletalHandSide& outSide,
    std::uint32_t& outBoneIndex
) noexcept;
DeviceDescriptor makeSkeletalBoneDeviceDescriptor(SkeletalHandSide side, std::uint32_t boneIndex);
std::string skeletalBoneName(std::uint32_t boneIndex);
std::string skeletalBoneNodeName(SkeletalHandSide side, std::uint32_t boneIndex);
bool skeletalBoneParentIndex(std::uint32_t boneIndex, std::uint32_t& outParentIndex) noexcept;
bool isSkeletalAuxBoneIndex(std::uint32_t boneIndex) noexcept;
bool shouldRecordSkeletalBoneIndex(std::uint32_t boneIndex) noexcept;

} // namespace ovtr
