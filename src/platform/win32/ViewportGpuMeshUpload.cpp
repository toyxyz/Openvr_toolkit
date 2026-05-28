#include "platform/win32/ViewportGpuMeshUpload.h"

#include "platform/win32/ViewportGpuMeshPlan.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ovtr::win32 {
namespace {

struct GpuMatcapVertex {
    std::array<float, 3> position{};
    std::array<float, 3> normal{};
};

GLenum glIndexType(const ViewportGpuIndexFormat format) noexcept
{
    return format == ViewportGpuIndexFormat::UnsignedShort ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
}

bool ensureBufferFunctions(ViewportGpuMesh& gpuMesh)
{
    if (glad_glGenBuffers == nullptr || glad_glBindBuffer == nullptr || glad_glBufferData == nullptr) {
        gpuMesh.uploadFailed = true;
        gpuMesh.failureReason = "OpenGL buffer functions are unavailable";
        return false;
    }
    return true;
}

bool createGpuBuffers(ViewportGpuMesh& gpuMesh)
{
    GLuint buffers[2]{};
    glad_glGenBuffers(2, buffers);
    if (buffers[0] == 0 || buffers[1] == 0) {
        gpuMesh.uploadFailed = true;
        gpuMesh.failureReason = "glGenBuffers failed";
        return false;
    }
    gpuMesh.vertexBuffer.reset(buffers[0]);
    gpuMesh.indexBuffer.reset(buffers[1]);
    return true;
}

template <typename IndexVector>
bool uploadGpuMeshData(
    ViewportGpuMesh& gpuMesh,
    const std::vector<GpuMatcapVertex>& vertices,
    const IndexVector& indices,
    const ViewportGpuMeshUploadPlan& plan
)
{
    if (!ensureBufferFunctions(gpuMesh) || !createGpuBuffers(gpuMesh)) {
        return false;
    }

    glad_glBindBuffer(GL_ARRAY_BUFFER, gpuMesh.vertexBuffer.get());
    glad_glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<ptrdiff_t>(vertices.size() * sizeof(GpuMatcapVertex)),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glad_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpuMesh.indexBuffer.get());
    glad_glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<ptrdiff_t>(indices.size() * sizeof(typename IndexVector::value_type)),
        indices.data(),
        GL_STATIC_DRAW
    );

    glad_glBindBuffer(GL_ARRAY_BUFFER, 0);
    glad_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    gpuMesh.indexType = glIndexType(plan.indexFormat);
    gpuMesh.indexCount = static_cast<GLsizei>(plan.indexCount);
    return true;
}

} // namespace

bool uploadImportedGltfGpuMesh(ViewportGpuMesh& gpuMesh, const ovtr::RenderModelGeometry& mesh)
{
    if (gpuMesh.vertexBuffer && gpuMesh.indexBuffer) {
        return true;
    }
    if (gpuMesh.uploadFailed) {
        return false;
    }

    const ViewportGpuMeshUploadPlan plan = planImportedGltfGpuMeshUpload(mesh);
    if (!plan.valid) {
        gpuMesh.uploadFailed = true;
        gpuMesh.failureReason = plan.error;
        return false;
    }

    std::vector<GpuMatcapVertex> vertices;
    vertices.reserve(mesh.vertices.size());
    for (const ovtr::RenderModelVertex& source : mesh.vertices) {
        vertices.push_back({source.position, source.normal});
    }
    return uploadGpuMeshData(gpuMesh, vertices, mesh.indices, plan);
}

bool uploadRenderModelGpuMesh(ViewportGpuMesh& gpuMesh, const RenderModelMesh& mesh)
{
    if (gpuMesh.vertexBuffer && gpuMesh.indexBuffer) {
        return true;
    }
    if (gpuMesh.uploadFailed) {
        return false;
    }

    const ViewportGpuMeshUploadPlan plan =
        planRenderModelGpuMeshUpload(mesh.vertices.size(), mesh.indices);
    if (!plan.valid) {
        gpuMesh.uploadFailed = true;
        gpuMesh.failureReason = plan.error;
        return false;
    }

    std::vector<GpuMatcapVertex> vertices;
    vertices.reserve(mesh.vertices.size());
    for (const RenderModelVertex& source : mesh.vertices) {
        vertices.push_back({source.position, source.normal});
    }
    return uploadGpuMeshData(gpuMesh, vertices, mesh.indices, plan);
}

} // namespace ovtr::win32
