#include "platform/win32/ViewportImportedScenePrimitives.h"

#include "platform/win32/ViewportGlStateScope.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <gl/GL.h>

#include <cstdint>

namespace ovtr::win32 {

void drawImportedGltfMeshTriangles(const ovtr::RenderModelGeometry& mesh)
{
    glBegin(GL_TRIANGLES);
    for (const std::uint16_t index : mesh.indices) {
        if (index >= mesh.vertices.size()) {
            continue;
        }
        const ovtr::RenderModelVertex& vertex = mesh.vertices[index];
        glNormal3f(vertex.normal[0], vertex.normal[1], vertex.normal[2]);
        glVertex3f(vertex.position[0], vertex.position[1], vertex.position[2]);
    }
    glEnd();
}

void drawImportedFallbackMarker3D()
{
    constexpr float radius = 0.025f;
    constexpr float axisLength = 0.10f;

    glBegin(GL_QUADS);
    glVertex3f(-radius, -radius, radius);
    glVertex3f(radius, -radius, radius);
    glVertex3f(radius, radius, radius);
    glVertex3f(-radius, radius, radius);

    glVertex3f(-radius, -radius, -radius);
    glVertex3f(-radius, radius, -radius);
    glVertex3f(radius, radius, -radius);
    glVertex3f(radius, -radius, -radius);

    glVertex3f(-radius, radius, -radius);
    glVertex3f(-radius, radius, radius);
    glVertex3f(radius, radius, radius);
    glVertex3f(radius, radius, -radius);

    glVertex3f(-radius, -radius, -radius);
    glVertex3f(radius, -radius, -radius);
    glVertex3f(radius, -radius, radius);
    glVertex3f(-radius, -radius, radius);

    glVertex3f(radius, -radius, -radius);
    glVertex3f(radius, radius, -radius);
    glVertex3f(radius, radius, radius);
    glVertex3f(radius, -radius, radius);

    glVertex3f(-radius, -radius, -radius);
    glVertex3f(-radius, -radius, radius);
    glVertex3f(-radius, radius, radius);
    glVertex3f(-radius, radius, -radius);
    glEnd();

    ScopedGlLineWidth lineWidth(1.25f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(axisLength, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, axisLength, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, axisLength);
    glEnd();
}

} // namespace ovtr::win32
