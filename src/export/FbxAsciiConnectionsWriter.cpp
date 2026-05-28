#include "export/FbxAsciiWriter.h"

#include <cstdint>
#include <ostream>
#include <vector>

namespace ovtr {

void writeFbxConnections(
    std::ostream& out,
    const std::vector<FbxDeviceExport>& devices,
    const std::int64_t rootModelId,
    const std::int64_t stackId,
    const std::int64_t layerId
)
{
    out << "Connections:  {\n";
    out << "    C: \"OO\"," << rootModelId << ",0\n";
    out << "    C: \"OO\"," << layerId << "," << stackId << "\n";
    for (const FbxDeviceExport& device : devices) {
        out << "    C: \"OO\"," << device.modelId << "," << rootModelId << "\n";
        if (device.geometry.available) {
            out << "    C: \"OO\"," << device.geometryId << "," << device.modelId << "\n";
        }
        out << "    C: \"OO\"," << device.translationNodeId << "," << layerId << "\n";
        out << "    C: \"OP\"," << device.translationNodeId << "," << device.modelId << ",\"Lcl Translation\"\n";
        out << "    C: \"OO\"," << device.rotationNodeId << "," << layerId << "\n";
        out << "    C: \"OP\"," << device.rotationNodeId << "," << device.modelId << ",\"Lcl Rotation\"\n";
        for (int axis = 0; axis < 3; ++axis) {
            out << "    C: \"OP\"," << device.translationCurveIds[axis] << "," << device.translationNodeId
                << ",\"d|" << static_cast<char>('X' + axis) << "\"\n";
            out << "    C: \"OP\"," << device.rotationCurveIds[axis] << "," << device.rotationNodeId
                << ",\"d|" << static_cast<char>('X' + axis) << "\"\n";
        }
    }
    out << "}\n";
}

} // namespace ovtr
