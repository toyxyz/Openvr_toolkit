#include "TestCases.h"
#include "TestSupport.h"
#include "FbxTestSupport.h"

#include "export/FbxAsciiExporter.h"

#include <filesystem>
#include <limits>
#include <string>

namespace ovtr::test {

void testFbxExportSampleRate()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_fbx_sample_rate_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    const std::filesystem::path fbxPath = testDir / "export.fbx";

    ovtr::FbxExportOptions options;
    options.outputPath = fbxPath;
    options.includeGeometry = false;
    options.coordinatePolicy = ovtr::FbxCoordinatePolicy::OpenVRNative;
    options.exportSampleRate = 60.0;

    const std::string fbx = exportFbxAsciiForTest(
        testDir,
        makeTestFrames(4),
        {makeTestTracker("LHR-SAMPLE-RATE")},
        options,
        "fbx-sample-rate-test",
        "FBX Sample Rate Test",
        "FBX sample-rate"
    );
    require(fbx.find("P: \"TimeMode\", \"enum\", \"\", \"\",3") != std::string::npos, "FBX sample-rate export should declare 60 FPS time mode");
    require(fbx.find("P: \"CustomFrameRate\", \"double\", \"Number\", \"\",-1") != std::string::npos, "FBX standard 60 FPS should not use a custom frame rate");
    require(fbx.find("P: \"LocalStop\", \"KTime\", \"Time\", \"\",1539538585") != std::string::npos, "FBX animation stack should declare the resampled time span");
    const std::string translationXCurve = findFbxObjectSection(fbx, "AnimCurve::Tracker_LHR_SAMPLE_RATE_T_X");
    require(translationXCurve.find("KeyTime: *3") != std::string::npos, "FBX sample-rate export should write 3 keys");
    require(translationXCurve.find("KeyValueFloat: *3") != std::string::npos, "FBX sample-rate values should have 3 keys");
    require(
        translationXCurve.find("a: 1.000000000,2.500") != std::string::npos,
        "FBX sample-rate export should interpolate middle translation key"
    );
    require(
        translationXCurve.find(",4.000000000") != std::string::npos,
        "FBX sample-rate export should keep final translation key"
    );

    ovtr::FbxExportOptions invalidOptions = options;
    invalidOptions.outputPath = testDir / "invalid_sample_rate.fbx";
    invalidOptions.exportSampleRate = 1001.0;
    const ovtr::RecordingSession session = makeTestSession(
        "fbx-invalid-sample-rate-test",
        "FBX Invalid Sample Rate Test",
        testDir / "frames.bin",
        testDir / "frame_index.bin",
        {makeTestTracker("LHR-SAMPLE-RATE")}
    );
    const ovtr::ExportResult invalidResult = ovtr::exportSessionToFbxAscii(session, invalidOptions);
    require(!invalidResult.success, "FBX export should reject sample rates above 1000 FPS");
    require(
        invalidResult.error.find("sample rate") != std::string::npos,
        "FBX invalid sample-rate error mismatch: " + invalidResult.error
    );

    std::filesystem::remove_all(testDir, ignored);
}

} // namespace ovtr::test
