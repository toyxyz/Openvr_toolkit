#include "export/FbxAsciiWriter.h"

#include "export/FbxAsciiFormatting.h"

#include <cstdint>
#include <ostream>
#include <vector>

namespace ovtr {
namespace {

std::vector<double> collectGeometryVertices(const RenderModelGeometry& geometry)
{
    std::vector<double> values;
    values.reserve(geometry.vertices.size() * 3);
    for (const RenderModelVertex& vertex : geometry.vertices) {
        values.push_back(vertex.position[0]);
        values.push_back(vertex.position[1]);
        values.push_back(vertex.position[2]);
    }
    return values;
}

std::vector<double> collectGeometryNormals(const RenderModelGeometry& geometry)
{
    std::vector<double> values;
    values.reserve(geometry.indices.size() * 3);
    for (const std::uint16_t index : geometry.indices) {
        if (index >= geometry.vertices.size()) {
            continue;
        }
        const RenderModelVertex& vertex = geometry.vertices[index];
        values.push_back(vertex.normal[0]);
        values.push_back(vertex.normal[1]);
        values.push_back(vertex.normal[2]);
    }
    return values;
}

std::vector<std::int32_t> collectPolygonVertexIndex(const RenderModelGeometry& geometry)
{
    std::vector<std::int32_t> values;
    values.reserve(geometry.indices.size());
    for (std::size_t i = 0; i + 2 < geometry.indices.size(); i += 3) {
        values.push_back(static_cast<std::int32_t>(geometry.indices[i]));
        values.push_back(static_cast<std::int32_t>(geometry.indices[i + 1]));
        values.push_back(-static_cast<std::int32_t>(geometry.indices[i + 2]) - 1);
    }
    return values;
}

} // namespace

void writeFbxGeometry(std::ostream& out, const FbxDeviceExport& device)
{
    if (!device.geometry.available) {
        return;
    }

    const std::vector<double> vertices = collectGeometryVertices(device.geometry);
    const std::vector<std::int32_t> polygonVertexIndex = collectPolygonVertexIndex(device.geometry);
    const std::vector<double> normals = collectGeometryNormals(device.geometry);

    out << "    Geometry: " << device.geometryId << ", \"Geometry::" << escapeFbxString(device.nodeName)
        << "_Geometry\", \"Mesh\" {\n";
    out << "        Vertices: *" << vertices.size() << " {\n";
    out << "            a: " << joinedDoubles(vertices) << "\n";
    out << "        }\n";
    out << "        PolygonVertexIndex: *" << polygonVertexIndex.size() << " {\n";
    out << "            a: " << joinedInt32(polygonVertexIndex) << "\n";
    out << "        }\n";
    out << "        LayerElementNormal: 0 {\n";
    out << "            Version: 101\n";
    out << "            Name: \"\"\n";
    out << "            MappingInformationType: \"ByPolygonVertex\"\n";
    out << "            ReferenceInformationType: \"Direct\"\n";
    out << "            Normals: *" << normals.size() << " {\n";
    out << "                a: " << joinedDoubles(normals) << "\n";
    out << "            }\n";
    out << "        }\n";
    out << "        Layer: 0 {\n";
    out << "            Version: 100\n";
    out << "            LayerElement:  {\n";
    out << "                Type: \"LayerElementNormal\"\n";
    out << "                TypedIndex: 0\n";
    out << "            }\n";
    out << "        }\n";
    out << "    }\n";
}

} // namespace ovtr
