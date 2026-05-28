#include "platform/win32/ViewportTriangleDisplayList.h"

#include "platform/win32/ViewportImportedScenePrimitives.h"
#include "platform/win32/ViewportRenderModelDraw.h"

#include <gl/GL.h>

namespace ovtr::win32 {
namespace {

template <typename DrawFn>
bool callOrBuildDisplayList(ViewportTriangleDisplayListCache& cache, DrawFn draw)
{
    if (cache.displayList) {
        glCallList(cache.displayList.get());
        return true;
    }

    if (cache.buildFailed) {
        return false;
    }

    const GLuint list = glGenLists(1);
    if (list == 0) {
        cache.buildFailed = true;
        return false;
    }

    glNewList(list, GL_COMPILE);
    draw();
    glEndList();

    cache.displayList.reset(list, 1);
    glCallList(cache.displayList.get());
    return true;
}

} // namespace

void resetTriangleDisplayListCache(ViewportTriangleDisplayListCache& cache) noexcept
{
    cache.displayList.reset();
    cache.buildFailed = false;
}

bool drawCachedImportedGltfMeshTriangles(
    ViewportTriangleDisplayListCache& cache,
    const ovtr::RenderModelGeometry& mesh
)
{
    return callOrBuildDisplayList(cache, [&mesh]() {
        drawImportedGltfMeshTriangles(mesh);
    });
}

bool drawCachedRenderModelTriangles(
    ViewportTriangleDisplayListCache& cache,
    const RenderModelMesh& mesh
)
{
    return callOrBuildDisplayList(cache, [&mesh]() {
        drawRenderModelTriangles(mesh);
    });
}

} // namespace ovtr::win32
