#include "TestCases.h"
#include "TestSupport.h"

#include "math/QuaternionUtils.h"
#include "recording/BinarySessionReader.h"
#include "recording/BinarySessionWriter.h"

#include <cmath>
#include <filesystem>

namespace ovtr::test {

void testBinarySessionRoundTrip()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_core_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    const std::filesystem::path framesPath = testDir / "frames.bin";
    const std::filesystem::path indexPath = testDir / "frame_index.bin";

    ovtr::BinarySessionWriter writer;
    require(writer.open(framesPath, indexPath), "writer open failed: " + writer.lastError());
    require(writer.appendFrame(makeTestFrame(0)), "append frame 0 failed: " + writer.lastError());
    require(writer.appendFrame(makeTestFrame(1)), "append frame 1 failed: " + writer.lastError());
    require(writer.frameCount() == 2, "writer frame count mismatch");
    writer.close();

    ovtr::BinarySessionReader reader;
    require(reader.open(framesPath, indexPath), "reader open failed: " + reader.lastError());
    require(reader.frameCount() == 2, "reader frame count mismatch");

    ovtr::FrameSample frame;
    require(reader.readFrame(1, frame), "reader failed to read frame 1: " + reader.lastError());
    require(frame.frameIndex == 1, "read frame index mismatch");
    require(frame.poses.size() == 1, "read pose count mismatch");
    require(frame.poses[0].deviceId == 1, "read pose device id mismatch");
    require(std::fabs(frame.poses[0].position[0] - 2.0f) < 0.0001f, "read pose position mismatch");
    require(ovtr::isNearlyUnitQuaternion(frame.poses[0].rotation), "read quaternion should be normalized");

    reader.close();
    std::filesystem::remove_all(testDir, ignored);
}

} // namespace ovtr::test
