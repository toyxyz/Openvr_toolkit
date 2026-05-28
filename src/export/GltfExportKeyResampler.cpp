#include "export/GltfExportSceneParts.h"

#include "math/PoseInterpolation.h"

#include <vector>

namespace ovtr {
namespace {

GltfKey interpolateKey(const GltfKey& from, const GltfKey& to, const double timeSeconds)
{
    const double duration = to.timeSeconds - from.timeSeconds;
    const double factor = duration > 0.0 ? clamp01((timeSeconds - from.timeSeconds) / duration) : 0.0;

    GltfKey key;
    key.timeSeconds = timeSeconds;
    key.translation = lerpVec3(from.translation, to.translation, factor);
    key.rotation = slerpQuaternion(from.rotation, to.rotation, factor);
    return key;
}

void makeQuaternionKeysContinuous(std::vector<GltfKey>& keys)
{
    for (std::size_t index = 1; index < keys.size(); ++index) {
        if (quaternionDot(keys[index - 1].rotation, keys[index].rotation) < 0.0) {
            for (float& component : keys[index].rotation) {
                component = -component;
            }
        }
    }
}

} // namespace

void resampleGltfKeys(std::vector<GltfKey>& keys, const double sampleRate)
{
    resampleKeyframesByTime(keys, sampleRate, interpolateKey);
    makeQuaternionKeysContinuous(keys);
}

} // namespace ovtr
