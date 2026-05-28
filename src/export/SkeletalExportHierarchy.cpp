#include "export/SkeletalExportHierarchy.h"

#include "data/SkeletalSyntheticPose.h"
#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <unordered_map>
#include <utility>

namespace ovtr {
namespace {

constexpr double kKeyTimeEpsilon = 0.000001;

bool decodeTrackBone(
    const ExportPoseTrack& track,
    SkeletalHandSide& side,
    std::uint32_t& boneIndex
) noexcept
{
    return decodeSkeletalBoneRuntimeIndex(track.device.runtimeIndex, side, boneIndex);
}

int sideRank(const SkeletalHandSide side) noexcept
{
    return side == SkeletalHandSide::Left ? 0 : 1;
}

const ExportPoseKey* findKeyAtTime(const std::vector<ExportPoseKey>& keys, const double timeSeconds)
{
    for (const ExportPoseKey& key : keys) {
        if (std::fabs(key.timeSeconds - timeSeconds) <= kKeyTimeEpsilon) {
            return &key;
        }
    }
    return nullptr;
}

ExportPoseKey makeLocalKey(const ExportPoseKey& childKey, const ExportPoseKey& parentKey)
{
    const std::array<float, 4> inverseParentRotation =
        conjugateQuaternion(normalizeQuaternion(parentKey.rotation));
    const std::array<float, 3> delta{
        childKey.translation[0] - parentKey.translation[0],
        childKey.translation[1] - parentKey.translation[1],
        childKey.translation[2] - parentKey.translation[2],
    };

    ExportPoseKey key;
    key.timeSeconds = childKey.timeSeconds;
    key.translation = rotatePositionByQuaternion(inverseParentRotation, delta);
    key.rotation = normalizeQuaternion(
        multiplyQuaternion(inverseParentRotation, normalizeQuaternion(childKey.rotation))
    );
    return key;
}

bool convertTrackToLocalKeys(
    ExportPoseTrack& track,
    const std::vector<ExportPoseKey>& childWorldKeys,
    const std::vector<ExportPoseKey>& parentWorldKeys,
    std::string& error
)
{
    std::vector<ExportPoseKey> localKeys;
    localKeys.reserve(childWorldKeys.size());
    for (const ExportPoseKey& childKey : childWorldKeys) {
        const ExportPoseKey* parentKey = findKeyAtTime(parentWorldKeys, childKey.timeSeconds);
        if (parentKey == nullptr) {
            error = "skeletal export hierarchy missing parent key time for " + track.nodeName;
            return false;
        }
        localKeys.push_back(makeLocalKey(childKey, *parentKey));
    }
    track.keys = std::move(localKeys);
    return true;
}

bool skeletalTrackLess(const ExportPoseTrack& left, const ExportPoseTrack& right)
{
    SkeletalHandSide leftSide = SkeletalHandSide::Left;
    SkeletalHandSide rightSide = SkeletalHandSide::Left;
    std::uint32_t leftBone = 0;
    std::uint32_t rightBone = 0;
    const bool leftSkeletal = decodeTrackBone(left, leftSide, leftBone);
    const bool rightSkeletal = decodeTrackBone(right, rightSide, rightBone);
    if (leftSkeletal != rightSkeletal) {
        return !leftSkeletal;
    }
    if (!leftSkeletal) {
        return false;
    }
    if (sideRank(leftSide) != sideRank(rightSide)) {
        return sideRank(leftSide) < sideRank(rightSide);
    }
    return leftBone < rightBone;
}

} // namespace

bool applySkeletalExportHierarchy(std::vector<ExportPoseTrack>& tracks, std::string& error)
{
    error.clear();

    std::unordered_map<std::uint32_t, std::size_t> runtimeToTrack;
    std::unordered_map<std::uint32_t, std::vector<ExportPoseKey>> worldKeys;
    runtimeToTrack.reserve(tracks.size());
    worldKeys.reserve(tracks.size());

    for (std::size_t index = 0; index < tracks.size(); ++index) {
        const std::uint32_t runtimeIndex = tracks[index].device.runtimeIndex;
        const auto inserted = runtimeToTrack.emplace(runtimeIndex, index);
        if (!inserted.second) {
            error = "duplicate runtime index while building skeletal export hierarchy";
            return false;
        }
        worldKeys.emplace(runtimeIndex, tracks[index].keys);
        tracks[index].hasParentRuntimeIndex = false;
        tracks[index].parentRuntimeIndex = 0;
    }

    for (ExportPoseTrack& track : tracks) {
        SkeletalHandSide side = SkeletalHandSide::Left;
        std::uint32_t boneIndex = 0;
        if (!decodeTrackBone(track, side, boneIndex)) {
            continue;
        }

        std::uint32_t parentBoneIndex = 0;
        if (!skeletalBoneParentIndex(boneIndex, parentBoneIndex)) {
            continue;
        }

        const std::uint32_t parentRuntime = skeletalBoneRuntimeIndex(side, parentBoneIndex);
        const auto parentFound = runtimeToTrack.find(parentRuntime);
        if (parentFound == runtimeToTrack.end()) {
            continue;
        }

        track.hasParentRuntimeIndex = true;
        track.parentRuntimeIndex = parentRuntime;
        if (!convertTrackToLocalKeys(track, worldKeys[track.device.runtimeIndex], worldKeys[parentRuntime], error)) {
            return false;
        }
    }

    std::stable_sort(tracks.begin(), tracks.end(), skeletalTrackLess);
    return true;
}

} // namespace ovtr
