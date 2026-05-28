#include "TestCases.h"
#include "TestSupport.h"

#include "export/GltfExporter.h"
#include "import/GltfImporter.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>

namespace ovtr::test {

void testGlbExport()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_glb_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    const std::filesystem::path framesPath = testDir / "frames.bin";
    const std::filesystem::path indexPath = testDir / "frame_index.bin";
    const std::filesystem::path glbPath = testDir / "export.glb";

    writeTestFrames(framesPath, indexPath, 2, "GLB");

    ovtr::DeviceDescriptor tracker = makeTestTracker("LHR-GLB");
    tracker.renderModelName = "test_render_model";

    const ovtr::RecordingSession session = makeTestSession(
        "glb-test",
        "GLB Test",
        framesPath,
        indexPath,
        {tracker}
    );

    ovtr::GltfExportOptions options;
    options.outputPath = glbPath;
    options.format = ovtr::GltfExportFormat::Glb;
    options.exportSampleRate = 60.0;
    options.geometryProvider = [](const ovtr::DeviceDescriptor&) {
        return makeTriangleGeometry();
    };
    const ovtr::ExportResult result = ovtr::exportSessionToGltf(session, options);
    require(result.success, "GLB export failed: " + result.error);

    const std::vector<std::uint8_t> glb = readBinaryFile(glbPath);
    require(glb.size() > 28, "GLB output should contain a header and chunks");
    require(readLittleEndianUint32(glb, 0) == 0x46546c67u, "GLB magic mismatch");
    require(readLittleEndianUint32(glb, 4) == 2u, "GLB version mismatch");
    require(readLittleEndianUint32(glb, 12) + 20 <= glb.size(), "GLB JSON chunk length invalid");
    require(readLittleEndianUint32(glb, 16) == 0x4e4f534au, "GLB first chunk should be JSON");

    const std::uint32_t jsonLength = readLittleEndianUint32(glb, 12);
    const std::string json(reinterpret_cast<const char*>(glb.data() + 20), jsonLength);
    require(json.find("\"path\": \"rotation\"") != std::string::npos, "GLB JSON should contain rotation animation");
    require(json.find("\"type\": \"VEC4\"") != std::string::npos, "GLB rotation accessor should be VEC4 quaternion");
    require(json.find("\"meshes\"") != std::string::npos, "GLB JSON should contain mesh data");
    require(json.find("\"mesh\": 0") != std::string::npos, "GLB device node should reference the mesh");
    require(json.find("\"POSITION\"") != std::string::npos, "GLB mesh should contain positions");
    require(json.find("\"NORMAL\"") != std::string::npos, "GLB mesh should contain normals");
    require(json.find("\"indices\"") != std::string::npos, "GLB mesh should contain indices");
    require(json.find("\"componentType\": 5123") != std::string::npos, "GLB mesh indices should be unsigned short");
    require(json.find("\"target\": 34963") != std::string::npos, "GLB index bufferView should use element-array target");
    require(json.find("\"uri\"") == std::string::npos, "GLB embedded buffer should not use an external URI");

    const ovtr::GltfImportResult importResult = ovtr::importGlbScene(glbPath);
    require(importResult.success, "GLB import failed: " + importResult.error);
    require(!importResult.scene.nodes.empty(), "GLB import should expose animated nodes");
    require(!importResult.scene.meshes.empty(), "GLB import should expose meshes");

    const ovtr::ImportedGltfNode* importedTracker = nullptr;
    for (const ovtr::ImportedGltfNode& node : importResult.scene.nodes) {
        if (node.meshIndex >= 0) {
            importedTracker = &node;
            break;
        }
    }
    require(importedTracker != nullptr, "GLB import should keep the mesh node");
    require(importedTracker->meshIndex == 0, "GLB imported node mesh index mismatch");
    require(importResult.scene.meshes[0].vertices.size() == 3, "GLB imported mesh vertex count mismatch");
    require(importResult.scene.meshes[0].indices.size() == 3, "GLB imported mesh index count mismatch");
    require(importedTracker->keys.size() >= 2, "GLB imported animation key count mismatch");

    std::array<float, 3> importedTranslation{};
    std::array<float, 4> importedRotation{};
    require(
        ovtr::sampleImportedGltfNodePose(*importedTracker, 0.0, importedTranslation, importedRotation),
        "GLB imported node should sample at start"
    );
    require(std::fabs(importedTranslation[0] - 1.0f) < 0.0001f, "GLB imported first translation mismatch");
    require(
        ovtr::sampleImportedGltfNodePose(
            *importedTracker,
            importResult.scene.durationSeconds,
            importedTranslation,
            importedRotation
        ),
        "GLB imported node should sample at end"
    );
    require(std::fabs(importedTranslation[0] - 2.0f) < 0.0001f, "GLB imported final translation mismatch");

    std::filesystem::remove_all(testDir, ignored);
}

} // namespace ovtr::test
