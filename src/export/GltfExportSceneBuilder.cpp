#include "export/GltfExportSceneBuilder.h"

#include "export/GltfExportSceneParts.h"

#include <utility>

namespace ovtr {

void buildGltfExportScene(
    std::vector<ExportPoseTrack> tracks,
    const double exportSampleRate,
    GltfExportScene& scene
)
{
    scene = {};
    scene.devices.reserve(tracks.size());

    int nextNodeIndex = 1;
    for (ExportPoseTrack& track : tracks) {
        GltfDevice device;
        device.device = std::move(track.device);
        device.nodeName = std::move(track.nodeName);
        device.nodeIndex = nextNodeIndex++;
        device.geometry = std::move(track.geometry);
        device.keys.reserve(track.keys.size());
        for (const ExportPoseKey& key : track.keys) {
            device.keys.push_back({
                key.timeSeconds,
                key.translation,
                key.rotation,
            });
        }
        scene.devices.push_back(std::move(device));
    }

    for (GltfDevice& device : scene.devices) {
        resampleGltfKeys(device.keys, exportSampleRate);
    }

    buildGltfMeshData(scene.devices, scene.binary, scene.bufferViews, scene.accessors, scene.meshes);
    buildGltfAnimationData(scene.devices, scene.binary, scene.bufferViews, scene.accessors, scene.animationTargets);
}

} // namespace ovtr
