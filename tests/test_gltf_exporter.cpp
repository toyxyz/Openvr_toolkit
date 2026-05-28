#include "TestCases.h"
#include "TestSupport.h"

#include "export/GltfExporter.h"

#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>

namespace ovtr::test {

void testGltfExport()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_gltf_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    const std::filesystem::path framesPath = testDir / "frames.bin";
    const std::filesystem::path indexPath = testDir / "frame_index.bin";
    const std::filesystem::path gltfPath = testDir / "export.gltf";
    const std::filesystem::path binPath = testDir / "export.bin";

    writeTestFrames(framesPath, indexPath, 2, "glTF");

    const std::string utf8DisplayName =
        std::string("\xED\x97\x88") + "\xEB\xA6\xAC " + "\xED\x8A\xB8" + "\xEB\x9E\x98" + "\xEC\xBB\xA4";
    const std::string utf8SafeName =
        std::string("\xED\x97\x88") + "\xEB\xA6\xAC_" + "\xED\x8A\xB8" + "\xEB\x9E\x98" + "\xEC\xBB\xA4";

    ovtr::DeviceDescriptor tracker = makeTestTracker("LHR-GLTF");
    tracker.displayName = utf8DisplayName;

    const ovtr::RecordingSession session = makeTestSession(
        "gltf-test",
        "glTF Test",
        framesPath,
        indexPath,
        {tracker}
    );

    ovtr::GltfExportOptions options;
    options.outputPath = gltfPath;
    options.format = ovtr::GltfExportFormat::Gltf;
    const ovtr::ExportResult result = ovtr::exportSessionToGltf(session, options);
    require(result.success, "glTF export failed: " + result.error);

    const std::string gltf = readTextFile(gltfPath);
    require(std::filesystem::exists(binPath), "glTF export should write sibling bin file");
    require(gltf.find("\"version\": \"2.0\"") != std::string::npos, "glTF asset version missing");
    require(gltf.find("\"uri\": \"export.bin\"") != std::string::npos, "glTF buffer URI missing");
    require(gltf.find("\"path\": \"rotation\"") != std::string::npos, "glTF rotation animation channel missing");
    require(gltf.find("\"type\": \"VEC4\"") != std::string::npos, "glTF rotation accessor should be VEC4 quaternion");
    require(gltf.find("\"interpolation\": \"LINEAR\"") != std::string::npos, "glTF animation interpolation missing");
    require(
        gltf.find(std::string("\"name\": \"") + utf8SafeName + "\"") != std::string::npos,
        "glTF node should keep UTF-8 bytes in safe custom display name"
    );
    require(gltf.find("\"serial\": \"LHR-GLTF\"") != std::string::npos, "glTF device extras should include serial");

    const std::vector<std::uint8_t> bin = readBinaryFile(binPath);
    require(!bin.empty(), "glTF binary buffer should not be empty");

    std::filesystem::remove_all(testDir, ignored);
}

void testGltfRejectsInvalidExportSampleRate()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_gltf_sample_rate_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    const std::filesystem::path framesPath = testDir / "frames.bin";
    const std::filesystem::path indexPath = testDir / "frame_index.bin";
    writeTestFrames(framesPath, indexPath, 2, "glTF invalid sample-rate");

    const ovtr::RecordingSession session = makeTestSession(
        "gltf-invalid-sample-rate-test",
        "glTF Invalid Sample Rate Test",
        framesPath,
        indexPath,
        {makeTestTracker("LHR-GLTF-SAMPLE-RATE")}
    );

    ovtr::GltfExportOptions options;
    options.outputPath = testDir / "invalid_sample_rate.glb";
    options.exportSampleRate = std::numeric_limits<double>::infinity();
    const ovtr::ExportResult result = ovtr::exportSessionToGltf(session, options);
    require(!result.success, "glTF export should reject non-finite sample rates");
    require(
        result.error.find("sample rate") != std::string::npos,
        "glTF invalid sample-rate error mismatch: " + result.error
    );

    std::filesystem::remove_all(testDir, ignored);
}

} // namespace ovtr::test
