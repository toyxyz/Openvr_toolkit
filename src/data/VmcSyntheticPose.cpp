#include "data/VmcSyntheticPose.h"

#include <array>

namespace ovtr {
namespace {

constexpr std::uint32_t kNoParentBone = UINT32_MAX;

std::uint32_t handOffset(const SkeletalHandSide side) noexcept
{
    return side == SkeletalHandSide::Left ? 0u : kVmcFingerBoneCount;
}

const char* handToken(const SkeletalHandSide side) noexcept
{
    return side == SkeletalHandSide::Left ? "Left" : "Right";
}

} // namespace

std::uint32_t vmcFingerRuntimeIndex(const SkeletalHandSide side, const std::uint32_t boneIndex)
{
    return kVmcFingerRuntimeIndexBase + handOffset(side) + boneIndex;
}

bool isVmcFingerRuntimeIndex(const std::uint32_t runtimeIndex) noexcept
{
    if (runtimeIndex < kVmcFingerRuntimeIndexBase) {
        return false;
    }
    const std::uint32_t offset = runtimeIndex - kVmcFingerRuntimeIndexBase;
    return offset < kVmcFingerBoneCount * 2u;
}

bool decodeVmcFingerRuntimeIndex(
    const std::uint32_t runtimeIndex,
    SkeletalHandSide& outSide,
    std::uint32_t& outBoneIndex
) noexcept {
    if (!isVmcFingerRuntimeIndex(runtimeIndex)) {
        return false;
    }
    const std::uint32_t offset = runtimeIndex - kVmcFingerRuntimeIndexBase;
    outSide = offset < kVmcFingerBoneCount ? SkeletalHandSide::Left : SkeletalHandSide::Right;
    outBoneIndex = offset % kVmcFingerBoneCount;
    return true;
}

std::string vmcFingerBoneName(const std::uint32_t boneIndex)
{
    static constexpr std::array<const char*, kVmcFingerBoneCount> kNames = {
        "Wrist",
        "Thumb_Proximal", "Thumb_Intermediate", "Thumb_Distal", "Thumb_Tip",
        "Index_Proximal", "Index_Intermediate", "Index_Distal", "Index_Tip",
        "Middle_Proximal", "Middle_Intermediate", "Middle_Distal", "Middle_Tip",
        "Ring_Proximal", "Ring_Intermediate", "Ring_Distal", "Ring_Tip",
        "Little_Proximal", "Little_Intermediate", "Little_Distal", "Little_Tip",
    };
    return boneIndex < kNames.size() ? kNames[boneIndex] : "Unknown";
}

std::string vmcFingerNodeName(const SkeletalHandSide side, const std::uint32_t boneIndex)
{
    return std::string("VMC_") + handToken(side) + "_" + vmcFingerBoneName(boneIndex);
}

bool vmcFingerParentIndex(const std::uint32_t boneIndex, std::uint32_t& outParentIndex) noexcept
{
    if (boneIndex == 0 || boneIndex >= kVmcFingerBoneCount) {
        return false;
    }
    const std::uint32_t local = (boneIndex - 1u) % 4u;
    outParentIndex = local == 0u ? 0u : boneIndex - 1u;
    return outParentIndex != kNoParentBone;
}

DeviceDescriptor makeVmcFingerDeviceDescriptor(const SkeletalHandSide side, const std::uint32_t boneIndex)
{
    DeviceDescriptor descriptor;
    descriptor.id = vmcFingerRuntimeIndex(side, boneIndex);
    descriptor.runtimeIndex = descriptor.id;
    descriptor.serial = std::string("OVTR-VMC-") + (side == SkeletalHandSide::Left ? "L-" : "R-") +
        vmcFingerBoneName(boneIndex);
    descriptor.displayName = vmcFingerNodeName(side, boneIndex);
    descriptor.role = side == SkeletalHandSide::Left ? "left_vmc_finger" : "right_vmc_finger";
    descriptor.deviceClass = DeviceClass::Other;
    descriptor.modelName = kVmcFingerBoxRenderModelName;
    descriptor.renderModelName = kVmcFingerBoxRenderModelName;
    descriptor.manufacturerName = "OpenVRTrackerRecorder";
    descriptor.recordEnabled = true;
    return descriptor;
}

} // namespace ovtr
