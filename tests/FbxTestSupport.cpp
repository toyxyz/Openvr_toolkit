#include "FbxTestSupport.h"

#include "TestSupport.h"

namespace ovtr::test {

std::string exportFbxAsciiForTest(
    const std::filesystem::path& testDir,
    const std::vector<ovtr::FrameSample>& frames,
    const std::vector<ovtr::DeviceDescriptor>& devices,
    ovtr::FbxExportOptions options,
    const std::string& sessionId,
    const std::string& sessionName,
    const std::string& errorPrefix
)
{
    const std::filesystem::path framesPath = testDir / "frames.bin";
    const std::filesystem::path indexPath = testDir / "frame_index.bin";
    const std::filesystem::path fbxPath = options.outputPath.empty()
        ? testDir / "export.fbx"
        : options.outputPath;

    writeFrameSamples(framesPath, indexPath, frames, errorPrefix);

    options.outputPath = fbxPath;
    const ovtr::RecordingSession session = makeTestSession(sessionId, sessionName, framesPath, indexPath, devices);
    const ovtr::ExportResult result = ovtr::exportSessionToFbxAscii(session, options);
    require(result.success, errorPrefix + " export failed: " + result.error);

    return readTextFile(fbxPath);
}

} // namespace ovtr::test
