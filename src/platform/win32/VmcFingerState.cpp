#include "platform/win32/VmcFingerState.h"

#include <array>

namespace ovtr::win32 {
namespace {

constexpr std::array<const char*, 5> kFingerNames = {
    "Thumb", "Index", "Middle", "Ring", "Little",
};
constexpr std::array<const char*, 3> kSegmentNames = {
    "Proximal", "Intermediate", "Distal",
};

bool equalsAscii(const std::string& left, const char* right) noexcept
{
    return left == right;
}

} // namespace

int vmcSideIndex(const ovtr::SkeletalHandSide side) noexcept
{
    return side == ovtr::SkeletalHandSide::Left ? 0 : 1;
}

bool isVmcFingerSideFresh(
    const VmcFingerSideState& state,
    const std::chrono::steady_clock::time_point now
) noexcept {
    return state.lastUpdate.time_since_epoch().count() != 0 &&
        std::chrono::duration<double>(now - state.lastUpdate).count() <= kVmcFingerFreshSeconds;
}

bool parseVmcFingerBoneName(
    const std::string& name,
    ovtr::SkeletalHandSide& outSide,
    std::uint32_t& outBoneIndex
) noexcept {
    for (int sideIndex = 0; sideIndex < 2; ++sideIndex) {
        const char* sideName = sideIndex == 0 ? "Left" : "Right";
        for (std::uint32_t finger = 0; finger < kFingerNames.size(); ++finger) {
            for (std::uint32_t segment = 0; segment < kSegmentNames.size(); ++segment) {
                const std::string candidate = std::string(sideName) +
                    kFingerNames[finger] + kSegmentNames[segment];
                if (equalsAscii(name, candidate.c_str())) {
                    outSide = sideIndex == 0 ? ovtr::SkeletalHandSide::Left : ovtr::SkeletalHandSide::Right;
                    outBoneIndex = finger * 3u + segment;
                    return true;
                }
            }
        }
    }
    return false;
}

std::uint32_t vmcPoseBoneIndexForNameBone(const std::uint32_t nameBoneIndex) noexcept
{
    const std::uint32_t finger = nameBoneIndex / 3u;
    const std::uint32_t segment = nameBoneIndex % 3u;
    return 1u + finger * 4u + segment;
}

} // namespace ovtr::win32
