#include "export/FbxExportSceneBuilder.h"

#include "export/FbxCoordinateConversion.h"
#include "export/FbxExportPoseKeys.h"
#include "export/FbxExportTimelineRanges.h"

#include <utility>

namespace ovtr {

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
