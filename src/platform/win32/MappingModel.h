#pragma once

#include "platform/win32/AppStateConstants.h"

#include <array>
#include <cstdint>
#include <string>

namespace ovtr::win32 {

inline constexpr int kMappingSlotCount = 11;
inline constexpr int kMappingPanelRowCount = kMappingSlotCount;
inline constexpr int kMappingArmSoftIkSlot = kMappingSlotCount;
inline constexpr int kMappingLegSoftIkSlot = kMappingSlotCount + 1;
inline constexpr const wchar_t* kMappingNoDeviceLabel = L"None";
inline constexpr float kDefaultMappingArmSoftIkStrength = 0.06f;
inline constexpr float kDefaultMappingLegSoftIkStrength = 0.03f;
inline constexpr float kDefaultMappingSoftIkStrength = kDefaultMappingArmSoftIkStrength;
inline constexpr std::array<float, 7> kMappingSoftIkStrengthOptions{
    0.0f,
    0.01f,
    kDefaultMappingLegSoftIkStrength,
    kDefaultMappingSoftIkStrength,
    0.10f,
    0.15f,
    0.25f
};

enum class MappingTrackerRole {
    Head,
    Chest,
    Pelvis,
    LeftArm,
    RightArm,
    LeftHand,
    RightHand,
    LeftLeg,
    RightLeg,
    LeftFoot,
    RightFoot,
};

struct MappingSlotDefinition {
    const wchar_t* label = L"";
    MappingTrackerRole role = MappingTrackerRole::Head;
};

constexpr std::array<std::uint32_t, kMappingSlotCount> defaultMappingDeviceRuntimeIndices() noexcept
{
    return {
        kNoSelectedRuntimeIndex,
        kNoSelectedRuntimeIndex,
        kNoSelectedRuntimeIndex,
        kNoSelectedRuntimeIndex,
        kNoSelectedRuntimeIndex,
        kNoSelectedRuntimeIndex,
        kNoSelectedRuntimeIndex,
        kNoSelectedRuntimeIndex,
        kNoSelectedRuntimeIndex,
        kNoSelectedRuntimeIndex,
        kNoSelectedRuntimeIndex
    };
}

const std::array<MappingSlotDefinition, kMappingSlotCount>& mappingSlotDefinitions() noexcept;
MappingTrackerRole mappingRoleForSlot(int slotIndex) noexcept;
int mappingSlotForRole(MappingTrackerRole role) noexcept;
bool isMappingDeviceRow(int rowIndex) noexcept;
bool isMappingSoftIkRow(int rowIndex) noexcept;
bool isMappingArmSoftIkRow(int rowIndex) noexcept;
const wchar_t* mappingPanelRowLabel(int rowIndex) noexcept;

} // namespace ovtr::win32
