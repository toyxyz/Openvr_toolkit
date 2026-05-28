#include "platform/win32/ViewportRenderModelDraw.h"

#include "platform/win32/ViewportMath.h"

#include <gl/GL.h>

#include <cstdint>

namespace ovtr::win32 {

void drawRenderModelTriangles(const RenderModelMesh& mesh)
{
    glBegin(GL_TRIANGLES);
    for (const std::uint16_t index : mesh.indices) {
        if (index >= mesh.vertices.size()) {
            continue;
        }
        const RenderModelVertex& vertex = mesh.vertices[index];
        glNormal3f(vertex.normal[0], vertex.normal[1], vertex.normal[2]);
        glTexCoord2f(vertex.texCoord[0], vertex.texCoord[1]);
        glVertex3f(vertex.position[0], vertex.position[1], vertex.position[2]);
    }
    glEnd();
}

void drawRenderModelOutlineTriangles(const RenderModelMesh& mesh, const float expansion)
{
    glBegin(GL_TRIANGLES);
    for (const std::uint16_t index : mesh.indices) {
        if (index >= mesh.vertices.size()) {
            continue;
        }
        const RenderModelVertex& vertex = mesh.vertices[index];
        const Vec3 normal = normalizeVec3({vertex.normal[0], vertex.normal[1], vertex.normal[2]});
        glVertex3f(
            vertex.position[0] + normal.x * expansion,
            vertex.position[1] + normal.y * expansion,
            vertex.position[2] + normal.z * expansion
        );
    }
    glEnd();
}

} // namespace ovtr::win32
