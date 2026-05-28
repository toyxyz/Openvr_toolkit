#include "export/FbxExportFileWriter.h"

#include "export/FbxAsciiWriter.h"
#include "export/FbxExportSceneBuilder.h"

#include <cstdint>
#include <ostream>
#include <string>

namespace ovtr {

bool writeFbxExportScene(
    std::ostream& out,
    const FbxCoordinatePolicy coordinatePolicy,
    FbxExportScene& scene,
    std::string& error
)
{
    const std::int64_t rootModelId = scene.nextId++;
    const std::int64_t stackId = scene.nextId++;
    const std::int64_t layerId = scene.nextId++;

    writeFbxHeader(out, coordinatePolicy, scene.timeline);
    out << "Objects:  {\n";
    writeFbxRootModel(out, rootModelId);
    for (const FbxDeviceExport& device : scene.devices) {
        writeFbxGeometry(out, device);
        writeFbxModel(out, device);
    }
    writeFbxAnimationStack(out, stackId, scene.timeline);
    out << "    AnimationLayer: " << layerId << ", \"AnimLayer::BaseLayer\", \"\" {\n";
    out << "    }\n";
    for (const FbxDeviceExport& device : scene.devices) {
        writeFbxCurveNode(out, device.translationNodeId, device.nodeName + "_T", false);
        writeFbxCurveNode(out, device.rotationNodeId, device.nodeName + "_R", true);
        for (int axis = 0; axis < 3; ++axis) {
            const std::string axisName(1, static_cast<char>('X' + axis));
            writeFbxCurve(
                out,
                device.translationCurveIds[axis],
                device.nodeName + "_T_" + axisName,
                device.keys,
                false,
                axis
            );
            writeFbxCurve(
                out,
                device.rotationCurveIds[axis],
                device.nodeName + "_R_" + axisName,
                device.keys,
                true,
                axis
            );
        }
    }
    out << "}\n";
    writeFbxConnections(out, scene.devices, rootModelId, stackId, layerId);

    if (!out.good()) {
        error = "failed while writing FBX output file";
        return false;
    }

    return true;
}

} // namespace ovtr
