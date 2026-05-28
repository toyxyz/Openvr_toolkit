#include "TestCases.h"
#include "TestSupport.h"
#include "GltfImportTestSupport.h"

#include "import/GltfImporter.h"
#include "import/GltfAccessorLayout.h"
#include "import/GltfAccessorReadLayout.h"
#include "util/BinaryBuffer.h"

#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

namespace ovtr::test {

void testGlbImportRejectsInvalidAccessor()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_import_invalid_accessor_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    const std::string json = minimalGltfJson(
        "[{\"bufferView\":0,\"componentType\":5126,\"count\":1,\"type\":\"MAT4\"}]",
        "[{\"primitives\":[{\"attributes\":{\"POSITION\":0},\"indices\":0}]}]"
    );
    const std::filesystem::path glbPath = testDir / "invalid_accessor.glb";
    writeTestGlb(glbPath, json, {0, 0, 0, 0}, true, true);

    const ovtr::GltfImportResult result = ovtr::importGlbScene(glbPath);
    require(!result.success, "GLB import should reject unsupported accessor layouts");
    require(
        result.error.find("accessor") != std::string::npos,
        "invalid-accessor GLB error mismatch: " + result.error
    );

    std::filesystem::remove_all(testDir, ignored);
}

void testGlbImportRejectsUnsafeAccessorLayout()
{
    std::string error;
    ovtr::GltfAccessorReadLayout layout;
    const std::vector<std::uint8_t> binary(64, 0);

    std::vector<ovtr::GltfBufferView> bufferViews{
        {0, std::numeric_limits<std::size_t>::max() - 3u, 8u, 0u}
    };
    std::vector<ovtr::GltfAccessor> accessors{
        {0, 0u, ovtr::kGltfComponentFloat, 1, "VEC3"}
    };
    require(
        !ovtr::resolveGltfAccessorReadLayout(bufferViews, accessors, 0, 3, binary, layout, error),
        "accessor layout should reject bufferView range overflow"
    );
    require(error.find("bufferView") != std::string::npos, "bufferView overflow error mismatch: " + error);

    error.clear();
    bufferViews = {{0, 0u, std::numeric_limits<std::size_t>::max(), 4u}};
    require(
        !ovtr::resolveGltfAccessorReadLayout(bufferViews, accessors, 0, 3, binary, layout, error),
        "accessor layout should reject byteStride smaller than element size"
    );
    require(error.find("byteStride") != std::string::npos, "byteStride error mismatch: " + error);

    error.clear();
    bufferViews = {{
        0,
        0u,
        std::numeric_limits<std::size_t>::max(),
        std::numeric_limits<std::size_t>::max()
    }};
    accessors = {{0, 0u, ovtr::kGltfComponentFloat, 2, "VEC3"}};
    require(
        !ovtr::resolveGltfAccessorReadLayout(bufferViews, accessors, 0, 3, binary, layout, error),
        "accessor layout should reject stride/count overflow"
    );
    require(error.find("overflows") != std::string::npos, "stride/count overflow error mismatch: " + error);
}

void testGlbImportAcceptsLargeIndex()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_import_large_index_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    constexpr std::size_t kVertexCount = 65537u;
    std::vector<std::uint8_t> binary;
    binary.reserve(kVertexCount * 12u + 12u);
    for (std::size_t i = 0; i < kVertexCount; ++i) {
        ovtr::appendLittleEndianFloat32(binary, static_cast<float>(i));
        ovtr::appendLittleEndianFloat32(binary, 0.0f);
        ovtr::appendLittleEndianFloat32(binary, 0.0f);
    }
    const std::size_t indexOffset = binary.size();
    ovtr::appendLittleEndianUint32(binary, 0u);
    ovtr::appendLittleEndianUint32(binary, 1u);
    ovtr::appendLittleEndianUint32(binary, 65536u);

    const std::string json = std::string("{")
        + "\"asset\":{\"version\":\"2.0\"},"
        + "\"buffers\":[{\"byteLength\":" + std::to_string(binary.size()) + "}],"
        + "\"bufferViews\":["
        + "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":" + std::to_string(indexOffset) + "},"
        + "{\"buffer\":0,\"byteOffset\":" + std::to_string(indexOffset) + ",\"byteLength\":12}"
        + "],"
        + "\"accessors\":["
        + "{\"bufferView\":0,\"componentType\":5126,\"count\":" + std::to_string(kVertexCount) + ",\"type\":\"VEC3\"},"
        + "{\"bufferView\":1,\"componentType\":5125,\"count\":3,\"type\":\"SCALAR\"}"
        + "],"
        + "\"nodes\":[{\"name\":\"Tracker\",\"mesh\":0}],"
        + "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},\"indices\":1}]}]"
        + "}";
    const std::filesystem::path glbPath = testDir / "large_index.glb";
    writeTestGlb(glbPath, json, binary, true, true);

    const ovtr::GltfImportResult result = ovtr::importGlbScene(glbPath);
    require(result.success, "GLB import should accept uint32 indices: " + result.error);
    require(result.scene.meshes.size() == 1, "large-index GLB mesh count mismatch");
    require(result.scene.meshes[0].vertices.size() == kVertexCount, "large-index GLB vertex count mismatch");
    require(result.scene.meshes[0].indices.size() == 3, "large-index GLB index count mismatch");
    require(result.scene.meshes[0].indices[2] == 65536u, "large-index GLB index value mismatch");

    std::filesystem::remove_all(testDir, ignored);
}

} // namespace ovtr::test
