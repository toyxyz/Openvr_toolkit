#include "export/FbxExportSceneBuilder.h"

#include "export/FbxCoordinateConversion.h"
#include "export/FbxExportPoseKeys.h"
#include "export/FbxExportTimelineRanges.h"

#include <cstddef>
#include <unordered_map>
#include <utility>

namespace ovtr {
namespace {

void linkFbxDeviceParents(std::vector<FbxDeviceExport>& devices)
{
    std::unordered_map<std::uint32_t, std::size_t> runtimeToDevice;
    runtimeToDevice.reserve(devices.size());
    for (std::size_t index = 0; index < devices.size(); ++index) {
        runtimeToDevice[devices[index].device.runtimeIndex] = index;
    }

    for (FbxDeviceExport& device : devices) {
        if (!device.hasParentRuntimeIndex) {
            continue;
        }
        const auto parentFound = runtimeToDevice.find(device.parentRuntimeIndex);
        if (parentFound == runtimeToDevice.end()) {
            continue;
        }
        device.parentModelId = devices[parentFound->second].modelId;
    }
}

} // namespace

void buildFbxExportScene(
    std::vector<ExportPoseTrack> tracks,
    const FbxCoordinatePolicy coordinatePolicy,
    const double exportSampleRate,
    const double targetSampleRate,
    FbxExportScene& scene
)
{
    scene = {};
    scene.devices.reserve(tracks.size());

    std::int64_t nextId = scene.nextId;
    for (ExportPoseTrack& track : tracks) {
        FbxDeviceExport device;
        device.device = std::move(track.device);
        device.nodeName = std::move(track.nodeName);
        device.hasParentRuntimeIndex = track.hasParentRuntimeIndex;
        device.parentRuntimeIndex = track.parentRuntimeIndex;
        device.modelId = nextId++;
        device.geometryId = nextId++;
        device.translationNodeId = nextId++;
        device.rotationNodeId = nextId++;
        for (int axis = 0; axis < 3; ++axis) {
            device.translationCurveIds[axis] = nextId++;
            device.rotationCurveIds[axis] = nextId++;
        }
        device.geometry = std::move(track.geometry);
        convertFbxGeometry(device.geometry, coordinatePolicy);
        device.keys.reserve(track.keys.size());
        for (const ExportPoseKey& sourceKey : track.keys) {
            device.keys.push_back(detail::makeFbxPoseKey(sourceKey, coordinatePolicy));
        }
        scene.devices.push_back(std::move(device));
    }
    linkFbxDeviceParents(scene.devices);

    for (FbxDeviceExport& device : scene.devices) {
        detail::resampleFbxPoseKeys(device.keys, exportSampleRate);
        detail::applyFbxEulerDiscontinuityFilter(device.keys);
    }

    scene.timeline = makeFbxTimelineSettings(
        detail::collectFbxTimelineKeyRanges(scene.devices),
        exportSampleRate,
        targetSampleRate
    );
    scene.nextId = nextId;
}

} // namespace ovtr
