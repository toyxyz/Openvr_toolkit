#include "export/GltfJsonBuilder.h"

#include "export/GltfJsonSections.h"

#include <iomanip>
#include <sstream>

namespace ovtr {

std::string makeGltfJson(
    const RecordingSession& session,
    const std::vector<GltfDevice>& devices,
    const std::vector<GltfMeshPrimitive>& meshes,
    const std::vector<GltfExportBufferView>& bufferViews,
    const std::vector<GltfExportAccessor>& accessors,
    const std::vector<GltfAnimationTarget>& animationTargets,
    const std::size_t binaryByteLength,
    const std::string& bufferUri
)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(9);
    out << "{\n";
    out << "  \"asset\": {\n";
    out << "    \"version\": \"2.0\",\n";
    out << "    \"generator\": \"OpenVR Tracker Recorder\"\n";
    out << "  },\n";
    out << "  \"scene\": 0,\n";
    out << "  \"scenes\": [\n";
    out << "    { \"name\": \"Recorded Motion\", \"nodes\": [0] }\n";
    out << "  ],\n";
    detail::writeGltfJsonNodes(out, devices);
    detail::writeGltfJsonMeshesAndMaterials(out, meshes);
    detail::writeGltfJsonAnimation(out, session, animationTargets);
    detail::writeGltfJsonBufferMetadata(out, bufferViews, accessors, binaryByteLength, bufferUri);
    detail::writeGltfJsonExtras(out, session);
    out << "}\n";
    return out.str();
}

} // namespace ovtr
