#include "export/GltfExportSceneBuilder.h"

#include "export/GltfExportSceneParts.h"

#include <cstddef>
#include <unordered_map>
#include <utility>

namespace ovtr {
namespace {

void linkGltfDeviceParents(std::vector<GltfDevice>& devices)
{
    std::unordered_map<std::uint32_t, std::size_t> runtimeToDevice;
    runtimeToDevice.reserve(devices.size());
    for (std::size_t index = 0; index < devices.size(); ++index) {
        runtimeToDevice[devices[index].device.runtimeIndex] = index;
    }

    for (GltfDevice& device : devices) {
        if (!device.hasParentRuntimeIndex) {
            continue;
        }
        const auto parentFound = runtimeToDevice.find(device.parentRuntimeIndex);
        if (parentFound == runtimeToDevice.end()) {
            continue;
        }
        GltfDevice& parent = devices[parentFound->second];
        device.parentNodeIndex = parent.nodeIndex;
        parent.children.push_back(device.nodeIndex);
    }
}

} // namespace

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
        device.hasParentRuntimeIndex = track.hasParentRuntimeIndex;
        device.parentRuntimeIndex = track.parentRuntimeIndex;
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
    linkGltfDeviceParents(scene.devices);

    for (GltfDevice& device : scene.devices) {
        resampleGltfKeys(device.keys, exportSampleRate);
    }

    buildGltfMeshData(scene.devices, scene.binary, scene.bufferViews, scene.accessors, scene.meshes);
    buildGltfAnimationData(scene.devices, scene.binary, scene.bufferViews, scene.accessors, scene.animationTargets);
}

} // namespace ovtr
