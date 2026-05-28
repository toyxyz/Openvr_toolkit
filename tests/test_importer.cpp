#include "TestCases.h"
#include "TestSupport.h"
#include "GltfImportTestSupport.h"

#include "import/GltfImporter.h"
#include "util/BinaryBuffer.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::test {

void testGlbImportRejectsMissingChunks()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_import_missing_chunks_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    const std::filesystem::path noJsonPath = testDir / "missing_json.glb";
    writeTestGlb(noJsonPath, "{}", {0, 0, 0, 0}, false, true);
    const ovtr::GltfImportResult noJson = ovtr::importGlbScene(noJsonPath);
    require(!noJson.success, "GLB import should reject files without a JSON chunk");
    require(noJson.error.find("JSON chunk") != std::string::npos, "missing-JSON GLB error mismatch: " + noJson.error);

    const std::filesystem::path noBinaryPath = testDir / "missing_binary.glb";
    writeTestGlb(noBinaryPath, "{}", {}, true, false);
    const ovtr::GltfImportResult noBinary = ovtr::importGlbScene(noBinaryPath);
    require(!noBinary.success, "GLB import should reject files without a BIN chunk");
    require(noBinary.error.find("binary chunk") != std::string::npos, "missing-BIN GLB error mismatch: " + noBinary.error);

    std::filesystem::remove_all(testDir, ignored);
}

void testGlbImportRejectsDeclaredLengthMismatch()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_import_length_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    const std::filesystem::path glbPath = testDir / "declared_length_mismatch.glb";
    std::vector<std::uint8_t> glb;
    ovtr::appendLittleEndianUint32(glb, 0x46546c67u);
    ovtr::appendLittleEndianUint32(glb, 2u);
    ovtr::appendLittleEndianUint32(glb, 12u);
    ovtr::appendLittleEndianUint32(glb, 4u);
    ovtr::appendLittleEndianUint32(glb, 0x4e4f534au);
    glb.push_back('{');
    glb.push_back('}');
    glb.push_back(' ');
    glb.push_back(' ');
    require(ovtr::writeFileBytes(glbPath, glb), "failed to write declared-length GLB");

    const ovtr::GltfImportResult result = ovtr::importGlbScene(glbPath);
    require(!result.success, "GLB import should reject declared length mismatch");
    require(
        result.error.find("length") != std::string::npos,
        "declared-length GLB error mismatch: " + result.error
    );

    std::filesystem::remove_all(testDir, ignored);
}

} // namespace ovtr::test
