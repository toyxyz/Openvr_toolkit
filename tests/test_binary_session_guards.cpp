#include "TestCases.h"
#include "TestSupport.h"

#include "recording/BinaryIO.h"
#include "recording/BinarySessionFormat.h"
#include "recording/BinarySessionReader.h"
#include "recording/BinarySessionWriter.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace ovtr::test {

void testBinarySessionRejectsTruncatedIndex()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_truncated_index_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    const std::filesystem::path framesPath = testDir / "frames.bin";
    const std::filesystem::path indexPath = testDir / "frame_index.bin";

    ovtr::BinarySessionWriter writer;
    require(writer.open(framesPath, indexPath), "truncated-index writer open failed: " + writer.lastError());
    require(writer.appendFrame(makeTestFrame(0)), "truncated-index append frame failed: " + writer.lastError());
    writer.close();

    std::ofstream corruptIndex(indexPath, std::ios::binary | std::ios::app);
    const char partialEntry = '\x7f';
    corruptIndex.write(&partialEntry, 1);
    corruptIndex.close();

    ovtr::BinarySessionReader reader;
    require(!reader.open(framesPath, indexPath), "reader should reject a partial frame index entry");
    require(
        reader.lastError().find("complete frame index entry") != std::string::npos,
        "truncated-index reader error mismatch: " + reader.lastError()
    );

    std::filesystem::remove_all(testDir, ignored);
}

void testBinarySessionRejectsUnreasonablePoseCount()
{
    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_pose_count_tests";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);
    std::filesystem::create_directories(testDir);

    const std::filesystem::path framesPath = testDir / "frames.bin";
    const std::filesystem::path indexPath = testDir / "frame_index.bin";

    std::ofstream frames(framesPath, std::ios::binary | std::ios::trunc);
    const ovtr::FrameFileHeader frameHeader;
    require(ovtr::writeBinaryArray(frames, frameHeader.magic), "failed to write test frame magic");
    require(ovtr::writeBinaryValue(frames, frameHeader.formatVersion), "failed to write test frame version");
    require(ovtr::writeBinaryValue(frames, frameHeader.endian), "failed to write test frame endian");
    require(ovtr::writeBinaryValue(frames, frameHeader.headerSize), "failed to write test frame header size");
    require(ovtr::writeBinaryValue(frames, frameHeader.reserved), "failed to write test frame reserved");

    const std::uint64_t frameIndex = 0;
    const std::uint64_t timestampNs = 0;
    const double timeSeconds = 0.0;
    const std::uint32_t poseCount = 257;
    const std::uint32_t frameFlags = 0;
    require(ovtr::writeBinaryValue(frames, frameIndex), "failed to write test frame index");
    require(ovtr::writeBinaryValue(frames, timestampNs), "failed to write test frame timestamp");
    require(ovtr::writeBinaryValue(frames, timeSeconds), "failed to write test frame time");
    require(ovtr::writeBinaryValue(frames, poseCount), "failed to write test pose count");
    require(ovtr::writeBinaryValue(frames, frameFlags), "failed to write test frame flags");
    frames.close();

    std::ofstream index(indexPath, std::ios::binary | std::ios::trunc);
    const ovtr::FrameIndexHeader indexHeader;
    require(ovtr::writeBinaryArray(index, indexHeader.magic), "failed to write test index magic");
    require(ovtr::writeBinaryValue(index, indexHeader.formatVersion), "failed to write test index version");
    require(ovtr::writeBinaryValue(index, indexHeader.entrySize), "failed to write test index entry size");
    require(ovtr::writeBinaryValue(index, frameIndex), "failed to write test index frame");
    require(ovtr::writeBinaryValue(index, timestampNs), "failed to write test index timestamp");
    const std::uint64_t byteOffset = frameHeader.headerSize;
    require(ovtr::writeBinaryValue(index, byteOffset), "failed to write test index offset");
    index.close();

    ovtr::BinarySessionReader reader;
    require(reader.open(framesPath, indexPath), "pose-count reader open failed: " + reader.lastError());
    ovtr::FrameSample frame;
    require(!reader.readFrame(0, frame), "reader should reject an unreasonable pose count before reserving");
    require(
        reader.lastError().find("pose count") != std::string::npos,
        "pose-count reader error mismatch: " + reader.lastError()
    );

    std::filesystem::remove_all(testDir, ignored);
}

} // namespace ovtr::test
