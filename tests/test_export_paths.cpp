#include "TestCases.h"
#include "TestSupport.h"

#include "data/SessionTypes.h"
#include "export/FbxAsciiExporter.h"
#include "export/GltfExporter.h"

#include <filesystem>

namespace ovtr::test {

void testExportFilenameOnlyPaths()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_filename_only_export_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    const std::filesystem::path framesPath = testDir / "frames.bin";
    const std::filesystem::path indexPath = testDir / "frame_index.bin";

    writeTestFrames(framesPath, indexPath, 1, "filename-only");

    const ovtr::DeviceDescriptor tracker = makeTestTracker("LHR-FILENAME");
    const ovtr::RecordingSession session = makeTestSession(
        "filename-only-export-test",
        "Filename Only Export Test",
        framesPath,
        indexPath,
        {tracker}
    );

    {
        const ScopedCurrentPath scopedPath(testDir);

        ovtr::FbxExportOptions fbxOptions;
        fbxOptions.outputPath = "filename_only.fbx";
        fbxOptions.includeGeometry = false;
        const ovtr::ExportResult fbxResult = ovtr::exportSessionToFbxAscii(session, fbxOptions);
        require(fbxResult.success, "filename-only FBX export failed: " + fbxResult.error);

        ovtr::GltfExportOptions gltfOptions;
        gltfOptions.outputPath = "filename_only.glb";
        gltfOptions.format = ovtr::GltfExportFormat::Glb;
        gltfOptions.includeGeometry = false;
        const ovtr::ExportResult gltfResult = ovtr::exportSessionToGltf(session, gltfOptions);
        require(gltfResult.success, "filename-only GLB export failed: " + gltfResult.error);
    }

    require(std::filesystem::exists(testDir / "filename_only.fbx"), "filename-only FBX output missing");
    require(std::filesystem::exists(testDir / "filename_only.glb"), "filename-only GLB output missing");

    std::filesystem::remove_all(testDir, ignored);
}

} // namespace ovtr::test
