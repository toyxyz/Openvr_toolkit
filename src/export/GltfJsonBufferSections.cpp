#include "export/GltfJsonSections.h"

#include "export/GltfJsonFormatting.h"
#include "util/JsonWriter.h"

#include <ostream>

namespace ovtr::detail {

void writeGltfJsonBufferMetadata(
    std::ostream& out,
    const std::vector<GltfExportBufferView>& bufferViews,
    const std::vector<GltfExportAccessor>& accessors,
    const std::size_t binaryByteLength,
    const std::string& bufferUri
)
{
    out << "  \"buffers\": [\n";
    out << "    { \"byteLength\": " << binaryByteLength;
    if (!bufferUri.empty()) {
        out << ", \"uri\": \"" << escapeJsonString(bufferUri) << "\"";
    }
    out << " }\n";
    out << "  ],\n";
    out << "  \"bufferViews\": [\n";
    for (std::size_t i = 0; i < bufferViews.size(); ++i) {
        const GltfExportBufferView& view = bufferViews[i];
        if (i != 0) {
            out << ",\n";
        }
        out << "    { \"buffer\": 0, \"byteOffset\": " << view.byteOffset << ", \"byteLength\": " << view.byteLength;
        if (view.target != 0) {
            out << ", \"target\": " << view.target;
        }
        out << " }";
    }
    out << "\n";
    out << "  ],\n";
    out << "  \"accessors\": [\n";
    for (std::size_t i = 0; i < accessors.size(); ++i) {
        const GltfExportAccessor& accessor = accessors[i];
        if (i != 0) {
            out << ",\n";
        }
        out << "    { \"bufferView\": " << accessor.bufferView << ", \"componentType\": " << accessor.componentType
            << ", \"count\": "
            << accessor.count << ", \"type\": \"" << accessor.type << "\"";
        if (!accessor.minValues.empty()) {
            out << ", \"min\": ";
            writeGltfJsonNumberArray(out, accessor.minValues);
        }
        if (!accessor.maxValues.empty()) {
            out << ", \"max\": ";
            writeGltfJsonNumberArray(out, accessor.maxValues);
        }
        out << " }";
    }
    out << "\n";
    out << "  ],\n";
}

void writeGltfJsonExtras(std::ostream& out, const RecordingSession& session)
{
    out << "  \"extras\": {\n";
    out << "    \"sessionId\": \"" << escapeJsonString(session.sessionId) << "\",\n";
    out << "    \"createdAtUtc\": \"" << escapeJsonString(session.createdAtUtc) << "\",\n";
    out << "    \"sourceCoordinateSystem\": \"" << escapeJsonString(session.coordinateSystem) << "\",\n";
    out << "    \"targetSampleRate\": " << formatGltfJsonDouble(session.targetSampleRate) << "\n";
    out << "  }\n";
}

} // namespace ovtr::detail
