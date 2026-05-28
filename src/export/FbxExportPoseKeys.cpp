#include "export/FbxExportPoseKeys.h"

#include "export/FbxAsciiMath.h"
#include "export/FbxCoordinateConversion.h"
#include "math/PoseInterpolation.h"

#include <array>

namespace ovtr::detail {

namespace {

FbxPoseKey makeInterpolatedPoseKey(const FbxPoseKey& from, const FbxPoseKey& to, const double timeSeconds)
{
    const double duration = to.timeSeconds - from.timeSeconds;
    const double factor = duration > 0.0 ? clamp01((timeSeconds - from.timeSeconds) / duration) : 0.0;

    FbxPoseKey key;
    key.timeSeconds = timeSeconds;
    key.translation = lerpVec3(from.translation, to.translation, factor);
    key.rotationQuaternion = slerpQuaternion(from.rotationQuaternion, to.rotationQuaternion, factor);
    key.rotationMatrix = fbxQuaternionToMatrix3(key.rotationQuaternion);
    key.rotationDegrees = fbxEulerRadiansToDegrees(fbxMatrix3ToEulerXyzRadians(key.rotationMatrix));
    return key;
}

} // namespace

FbxPoseKey makeFbxPoseKey(const ExportPoseKey& sourceKey, const FbxCoordinatePolicy coordinatePolicy)
{
    FbxPoseKey key;
    key.timeSeconds = sourceKey.timeSeconds;
    const std::array<float, 3> translation = convertFbxVector(sourceKey.translation, coordinatePolicy);
    key.translation = {
        static_cast<double>(translation[0]),
        static_cast<double>(translation[1]),
        static_cast<double>(translation[2]),
    };
    const std::array<float, 4> rotation = convertFbxQuaternion(sourceKey.rotation, coordinatePolicy);
    key.rotationQuaternion = rotation;
    key.rotationMatrix = fbxQuaternionToMatrix3(rotation);
    key.rotationDegrees = quaternionToEulerXyzDegrees(rotation);
    return key;
}

void resampleFbxPoseKeys(std::vector<FbxPoseKey>& keys, const double sampleRate)
{
    resampleKeyframesByTime(keys, sampleRate, makeInterpolatedPoseKey);
}

void applyFbxEulerDiscontinuityFilter(std::vector<FbxPoseKey>& keys)
{
    if (keys.size() < 2) {
        return;
    }

    std::vector<std::array<double, 3>> eulers;
    eulers.reserve(keys.size());
    for (const FbxPoseKey& key : keys) {
        eulers.push_back(fbxEulerDegreesToRadians(key.rotationDegrees));
    }

    for (std::size_t index = 1; index < eulers.size(); ++index) {
        eulers[index] = fbxCompatibleEulerXyzRadiansFromMatrix(keys[index].rotationMatrix, eulers[index - 1]);
    }

    unwrapFbxEulerRadians(eulers);

    for (std::size_t index = 0; index < keys.size(); ++index) {
        keys[index].rotationDegrees = fbxEulerRadiansToDegrees(eulers[index]);
    }
}

} // namespace ovtr::detail
