#include "platform/win32/MappingModel.h"

#include <cstddef>

namespace ovtr::win32 {

const std::array<MappingSlotDefinition, kMappingSlotCount>& mappingSlotDefinitions() noexcept
{
    static constexpr std::array<MappingSlotDefinition, kMappingSlotCount> definitions{{
        {L"Head", MappingTrackerRole::Head},
        {L"Chest", MappingTrackerRole::Chest},
        {L"Pelvis", MappingTrackerRole::Pelvis},
        {L"Left Arm", MappingTrackerRole::LeftArm},
        {L"Right Arm", MappingTrackerRole::RightArm},
        {L"Left hand", MappingTrackerRole::LeftHand},
        {L"Right hand", MappingTrackerRole::RightHand},
        {L"Left Leg", MappingTrackerRole::LeftLeg},
        {L"Right Leg", MappingTrackerRole::RightLeg},
        {L"Left Foot", MappingTrackerRole::LeftFoot},
        {L"Right Foot", MappingTrackerRole::RightFoot}
    }};
    return definitions;
}

MappingTrackerRole mappingRoleForSlot(const int slotIndex) noexcept
{
    if (slotIndex < 0 || slotIndex >= kMappingSlotCount) {
        return MappingTrackerRole::Head;
    }
    return mappingSlotDefinitions()[static_cast<std::size_t>(slotIndex)].role;
}

int mappingSlotForRole(const MappingTrackerRole role) noexcept
{
    const auto& definitions = mappingSlotDefinitions();
    for (int slot = 0; slot < kMappingSlotCount; ++slot) {
        if (definitions[static_cast<std::size_t>(slot)].role == role) {
            return slot;
        }
    }
    return 0;
}

bool isMappingDeviceRow(const int rowIndex) noexcept
{
    return rowIndex >= 0 && rowIndex < kMappingSlotCount;
}

bool isMappingFingerRow(const int rowIndex) noexcept
{
    return rowIndex == kMappingLeftFingerSlot || rowIndex == kMappingRightFingerSlot;
}

int mappingFingerSideIndexForRow(const int rowIndex) noexcept
{
    if (rowIndex == kMappingLeftFingerSlot) {
        return 0;
    }
    if (rowIndex == kMappingRightFingerSlot) {
        return 1;
    }
    return -1;
}

bool isMappingSoftIkRow(const int rowIndex) noexcept
{
    return rowIndex == kMappingArmSoftIkSlot || rowIndex == kMappingLegSoftIkSlot;
}

bool isMappingArmSoftIkRow(const int rowIndex) noexcept
{
    return rowIndex == kMappingArmSoftIkSlot;
}

const wchar_t* mappingPanelRowLabel(const int rowIndex) noexcept
{
    if (isMappingDeviceRow(rowIndex)) {
        return mappingSlotDefinitions()[static_cast<std::size_t>(rowIndex)].label;
    }
    if (isMappingArmSoftIkRow(rowIndex)) {
        return L"Arm Soft IK";
    }
    if (rowIndex == kMappingLeftFingerSlot) {
        return L"Left Finger";
    }
    if (rowIndex == kMappingRightFingerSlot) {
        return L"Right Finger";
    }
    if (isMappingSoftIkRow(rowIndex)) {
        return L"Leg Soft IK";
    }
    return L"";
}

} // namespace ovtr::win32
