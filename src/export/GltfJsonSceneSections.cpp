#include "export/GltfJsonSections.h"

#include "export/GltfJsonFormatting.h"
#include "util/JsonWriter.h"

#include <ostream>

namespace ovtr::detail {

void writeGltfJsonNodes(std::ostream& out, const std::vector<GltfDevice>& devices)
{
    out << "  \"nodes\": [\n";
    out << "    { \"name\": \"OpenVR_Root\"";
    if (!devices.empty()) {
        out << ", \"children\": [";
        for (std::size_t i = 0; i < devices.size(); ++i) {
            if (i != 0) {
                out << ",";
            }
            out << devices[i].nodeIndex;
        }
        out << "]";
    }
    out << " }";
    for (const GltfDevice& device : devices) {
        const GltfKey defaultKey = device.keys.empty() ? GltfKey{} : device.keys.front();
        out << ",\n";
        out << "    { \"name\": \"" << escapeJsonString(device.nodeName) << "\", ";
        out << "\"translation\": ";
        writeGltfJsonFloatArray(out, defaultKey.translation);
        out << ", \"rotation\": ";
        writeGltfJsonFloatArray(out, defaultKey.rotation);
        if (device.meshIndex >= 0) {
            out << ", \"mesh\": " << device.meshIndex;
        }
        out << ", \"extras\": {";
        out << "\"runtimeIndex\": " << device.device.runtimeIndex;
        out << ", \"serial\": \"" << escapeJsonString(device.device.serial) << "\"";
        out << ", \"deviceClass\": \"" << escapeJsonString(toString(device.device.deviceClass)) << "\"";
        if (!device.device.role.empty()) {
            out << ", \"role\": \"" << escapeJsonString(device.device.role) << "\"";
        }
        if (!device.device.modelName.empty()) {
            out << ", \"modelName\": \"" << escapeJsonString(device.device.modelName) << "\"";
        }
        out << "} }";
    }
    out << "\n";
    out << "  ],\n";
}

void writeGltfJsonMeshesAndMaterials(std::ostream& out, const std::vector<GltfMeshPrimitive>& meshes)
{
    if (meshes.empty()) {
        return;
    }

    out << "  \"meshes\": [\n";
    for (std::size_t i = 0; i < meshes.size(); ++i) {
        const GltfMeshPrimitive& mesh = meshes[i];
        if (i != 0) {
            out << ",\n";
        }
        out << "    { \"name\": \"" << escapeJsonString(mesh.name) << "\", \"primitives\": [";
        out << "{ \"attributes\": { \"POSITION\": " << mesh.positionAccessor << ", \"NORMAL\": "
            << mesh.normalAccessor << " }, \"indices\": " << mesh.indexAccessor
            << ", \"mode\": 4, \"material\": 0 }";
        out << "] }";
    }
    out << "\n";
    out << "  ],\n";
    out << "  \"materials\": [\n";
    out << "    { \"name\": \"OpenVR Device Default\", \"pbrMetallicRoughness\": { ";
    out << "\"baseColorFactor\": [0.800000000,0.820000000,0.860000000,1.000000000], ";
    out << "\"metallicFactor\": 0.000000000, \"roughnessFactor\": 0.650000000 } }\n";
    out << "  ],\n";
}

} // namespace ovtr::detail
