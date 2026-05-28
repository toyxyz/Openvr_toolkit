#include "TestCases.h"

#include "platform/win32/ViewportGpuMeshPlan.h"
#include "platform/win32/ViewportMatcapShader.h"
#include "TestSupport.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ovtr::test {

void testWin32ViewportGpuMesh()
{
    ovtr::RenderModelGeometry imported = makeTriangleGeometry();
    imported.available = true;
    auto importedPlan = ovtr::win32::planImportedGltfGpuMeshUpload(imported);
    require(importedPlan.valid, "imported GLB GPU upload plan accepts indexed geometry");
    require(
        importedPlan.indexFormat == ovtr::win32::ViewportGpuIndexFormat::UnsignedInt,
        "imported GLB GPU upload keeps 32-bit indices"
    );
    require(importedPlan.indexCount == 3, "imported GLB GPU upload reports index count");

    imported.indices = {0, 99, 1};
    importedPlan = ovtr::win32::planImportedGltfGpuMeshUpload(imported);
    require(!importedPlan.valid, "imported GLB GPU upload rejects invalid indices");

    const std::vector<std::uint16_t> renderModelIndices{0, 1, 2};
    const auto renderModelPlan =
        ovtr::win32::planRenderModelGpuMeshUpload(3, renderModelIndices);
    require(renderModelPlan.valid, "render model GPU upload accepts indexed geometry");
    require(
        renderModelPlan.indexFormat == ovtr::win32::ViewportGpuIndexFormat::UnsignedShort,
        "render model GPU upload uses 16-bit indices"
    );

    const std::vector<std::uint16_t> invalidRenderModelIndices{0, 4, 2};
    const auto invalidRenderModelPlan =
        ovtr::win32::planRenderModelGpuMeshUpload(3, invalidRenderModelIndices);
    require(!invalidRenderModelPlan.valid, "render model GPU upload rejects invalid indices");

    const std::string vertexShader = ovtr::win32::viewportMatcapVertexShaderSource();
    const std::string fragmentShader = ovtr::win32::viewportMatcapFragmentShaderSource();
    require(vertexShader.find("uModelViewProjection") != std::string::npos, "matcap vertex shader exposes MVP uniform");
    require(vertexShader.find("aNormal") != std::string::npos, "matcap vertex shader exposes normal attribute");
    require(fragmentShader.find("uMatcap") != std::string::npos, "matcap fragment shader exposes sampler uniform");
    require(fragmentShader.find("uTintColor") != std::string::npos, "matcap fragment shader exposes tint uniform");
}

} // namespace ovtr::test
