#include "export/GltfExportSceneParts.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace ovtr {
namespace {

double globalStartTime(const std::vector<GltfDevice>& devices)
{
    double start = 0.0;
    bool found = false;
    for (const GltfDevice& device : devices) {
        if (device.keys.empty()) {
            continue;
        }
        if (!found) {
            start = device.keys.front().timeSeconds;
            found = true;
        } else {
            start = std::min(start, device.keys.front().timeSeconds);
        }
    }
    return found ? start : 0.0;
}

} // namespace

void buildGltfAnimationData(
    std::vector<GltfDevice>& devices,
    std::vector<std::uint8_t>& binary,
    std::vector<GltfExportBufferView>& bufferViews,
    std::vector<GltfExportAccessor>& accessors,
    std::vector<GltfAnimationTarget>& animationTargets
)
{
    const double startTime = globalStartTime(devices);
    for (GltfDevice& device : devices) {
        if (device.keys.empty()) {
            continue;
        }

        std::vector<float> times;
        std::vector<float> translations;
        std::vector<float> rotations;
        times.reserve(device.keys.size());
        translations.reserve(device.keys.size() * 3);
        rotations.reserve(device.keys.size() * 4);

        float minTime = std::numeric_limits<float>::max();
        float maxTime = std::numeric_limits<float>::lowest();
        for (const GltfKey& key : device.keys) {
            const float time = static_cast<float>(std::max(0.0, key.timeSeconds - startTime));
            times.push_back(time);
            minTime = std::min(minTime, time);
            maxTime = std::max(maxTime, time);

            translations.push_back(key.translation[0]);
            translations.push_back(key.translation[1]);
            translations.push_back(key.translation[2]);

            rotations.push_back(key.rotation[0]);
            rotations.push_back(key.rotation[1]);
            rotations.push_back(key.rotation[2]);
            rotations.push_back(key.rotation[3]);
        }

        const int timeView = appendGltfFloatBufferView(binary, bufferViews, times);
        const int translationView = appendGltfFloatBufferView(binary, bufferViews, translations);
        const int rotationView = appendGltfFloatBufferView(binary, bufferViews, rotations);

        const int timeAccessor = addGltfAccessor(
            accessors,
            timeView,
            kGltfComponentFloat,
            static_cast<int>(times.size()),
            "SCALAR",
            {minTime},
            {maxTime}
        );
        const int translationAccessor = addGltfAccessor(
            accessors,
            translationView,
            kGltfComponentFloat,
            static_cast<int>(device.keys.size()),
            "VEC3"
        );
        const int rotationAccessor = addGltfAccessor(
            accessors,
            rotationView,
            kGltfComponentFloat,
            static_cast<int>(device.keys.size()),
            "VEC4"
        );

        animationTargets.push_back({
            device.nodeIndex,
            timeAccessor,
            translationAccessor,
            rotationAccessor,
        });
    }
}

} // namespace ovtr
