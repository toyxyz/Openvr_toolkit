#include "data/SkeletalSyntheticPose.h"

#include <array>

namespace ovtr {
namespace {

std::uint32_t handOffset(const SkeletalHandSide side) noexcept
{
    return side == SkeletalHandSide::Left ? 0u : kSkeletalHandBoneCount;
}

const char* handToken(const SkeletalHandSide side) noexcept
{
    return side == SkeletalHandSide::Left ? "Left" : "Right";
}

constexpr std::uint32_t kNoParentBone = UINT32_MAX;

} // namespace

std::uint32_t skeletalBoneRuntimeIndex(const SkeletalHandSide side, const std::uint32_t boneIndex)
{
    return kSkeletalSyntheticRuntimeIndexBase + handOffset(side) + boneIndex;
}

bool isSkeletalBoneRuntimeIndex(const std::uint32_t runtimeIndex) noexcept
{
    if (runtimeIndex < kSkeletalSyntheticRuntimeIndexBase) {
        return false;
    }
    const std::uint32_t offset = runtimeIndex - kSkeletalSyntheticRuntimeIndexBase;
    return offset < kSkeletalHandBoneCount * 2u;
}

bool decodeSkeletalBoneRuntimeIndex(
    const std::uint32_t runtimeIndex,
    SkeletalHandSide& outSide,
    std::uint32_t& outBoneIndex
) noexcept
{
    if (!isSkeletalBoneRuntimeIndex(runtimeIndex)) {
        return false;
    }

    const std::uint32_t offset = runtimeIndex - kSkeletalSyntheticRuntimeIndexBase;
    outSide = offset < kSkeletalHandBoneCount ? SkeletalHandSide::Left : SkeletalHandSide::Right;
    outBoneIndex = offset % kSkeletalHandBoneCount;
    return true;
}

std::string skeletalBoneName(const std::uint32_t boneIndex)
{
    static constexpr std::array<const char*, kSkeletalHandBoneCount> kBoneNames = {
        "Root",
        "Wrist",
        "Thumb_0",
        "Thumb_1",
        "Thumb_2",
        "Thumb_3",
        "Index_0",
        "Index_1",
        "Index_2",
        "Index_3",
        "Index_4",
        "Middle_0",
        "Middle_1",
        "Middle_2",
        "Middle_3",
        "Middle_4",
        "Ring_0",
        "Ring_1",
        "Ring_2",
        "Ring_3",
        "Ring_4",
        "Pinky_0",
        "Pinky_1",
        "Pinky_2",
        "Pinky_3",
        "Pinky_4",
        "Aux_Thumb",
        "Aux_Index",
        "Aux_Middle",
        "Aux_Ring",
        "Aux_Pinky",
    };
    return boneIndex < kBoneNames.size() ? kBoneNames[boneIndex] : "Unknown";
}

std::string skeletalBoneNodeName(const SkeletalHandSide side, const std::uint32_t boneIndex)
{
    return std::string("Skeletal_") + handToken(side) + "_" + skeletalBoneName(boneIndex);
}

bool skeletalBoneParentIndex(const std::uint32_t boneIndex, std::uint32_t& outParentIndex) noexcept
{
    static constexpr std::array<std::uint32_t, kSkeletalHandBoneCount> kParents = {
        kNoParentBone,
        0,
        1, 2, 3, 4,
        1, 6, 7, 8, 9,
        1, 11, 12, 13, 14,
        1, 16, 17, 18, 19,
        1, 21, 22, 23, 24,
        kNoParentBone, kNoParentBone, kNoParentBone, kNoParentBone, kNoParentBone,
    };

    if (boneIndex >= kParents.size() || kParents[boneIndex] == kNoParentBone) {
        return false;
    }
    outParentIndex = kParents[boneIndex];
    return true;
}

DeviceDescriptor makeSkeletalBoneDeviceDescriptor(const SkeletalHandSide side, const std::uint32_t boneIndex)
{
    DeviceDescriptor descriptor;
    descriptor.id = skeletalBoneRuntimeIndex(side, boneIndex);
    descriptor.runtimeIndex = descriptor.id;
    descriptor.serial = std::string("OVTR-SKEL-") + (side == SkeletalHandSide::Left ? "L-" : "R-") +
        skeletalBoneName(boneIndex);
    descriptor.displayName = skeletalBoneNodeName(side, boneIndex);
    descriptor.role = side == SkeletalHandSide::Left ? "left_skeletal_bone" : "right_skeletal_bone";
    descriptor.deviceClass = DeviceClass::Other;
    descriptor.modelName = kSkeletalBoneBoxRenderModelName;
    descriptor.renderModelName = kSkeletalBoneBoxRenderModelName;
    descriptor.manufacturerName = "OpenVRTrackerRecorder";
    descriptor.recordEnabled = true;
    return descriptor;
}

} // namespace ovtr
