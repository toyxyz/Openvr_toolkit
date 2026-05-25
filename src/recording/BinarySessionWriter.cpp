#include "recording/BinarySessionWriter.h"

#include "recording/BinaryIO.h"

#include <filesystem>
#include <utility>

namespace ovtr {

BinarySessionWriter::~BinarySessionWriter()
{
    close();
}

bool BinarySessionWriter::open(const std::filesystem::path& framesPath, const std::filesystem::path& indexPath)
{
    close();
    lastError_.clear();

    std::error_code error;
    if (!framesPath.parent_path().empty()) {
        std::filesystem::create_directories(framesPath.parent_path(), error);
        if (error) {
            setError("failed to create frame directory: " + error.message());
            return false;
        }
    }

    if (!indexPath.parent_path().empty()) {
        std::filesystem::create_directories(indexPath.parent_path(), error);
        if (error) {
            setError("failed to create index directory: " + error.message());
            return false;
        }
    }

    frames_.open(framesPath, std::ios::binary | std::ios::trunc);
    if (!frames_.is_open()) {
        setError("failed to open frame file");
        return false;
    }

    index_.open(indexPath, std::ios::binary | std::ios::trunc);
    if (!index_.is_open()) {
        setError("failed to open index file");
        close();
        return false;
    }

    frameCount_ = 0;
    return writeFrameHeader() && writeIndexHeader();
}

bool BinarySessionWriter::appendFrame(const FrameSample& frame)
{
    if (!isOpen()) {
        setError("writer is not open");
        return false;
    }

    const auto byteOffsetPosition = frames_.tellp();
    if (byteOffsetPosition < std::streampos(0)) {
        setError("failed to query frame file offset");
        return false;
    }

    const auto byteOffset = static_cast<std::uint64_t>(byteOffsetPosition - std::streampos(0));
    const auto poseCount = static_cast<std::uint32_t>(frame.poses.size());
    const std::uint32_t frameFlags = 0;

    if (!writeBinaryValue(frames_, frame.frameIndex) ||
        !writeBinaryValue(frames_, frame.timestampNs) ||
        !writeBinaryValue(frames_, frame.timeSeconds) ||
        !writeBinaryValue(frames_, poseCount) ||
        !writeBinaryValue(frames_, frameFlags)) {
        setError("failed to write frame record header");
        return false;
    }

    for (const PoseSample& pose : frame.poses) {
        if (!writeBinaryValue(frames_, pose.deviceId) ||
            !writeBinaryValue(frames_, pose.runtimeIndex) ||
            !writeBinaryArray(frames_, pose.position) ||
            !writeBinaryArray(frames_, pose.rotation) ||
            !writeBinaryArray(frames_, pose.velocity) ||
            !writeBinaryArray(frames_, pose.angularVelocity) ||
            !writeBinaryValue(frames_, pose.flags)) {
            setError("failed to write pose record");
            return false;
        }
    }

    const FrameIndexEntry entry{frame.frameIndex, frame.timestampNs, byteOffset};
    if (!writeBinaryValue(index_, entry.frameIndex) ||
        !writeBinaryValue(index_, entry.timestampNs) ||
        !writeBinaryValue(index_, entry.byteOffset)) {
        setError("failed to write frame index entry");
        return false;
    }

    ++frameCount_;
    return true;
}

void BinarySessionWriter::close()
{
    if (frames_.is_open()) {
        frames_.flush();
        frames_.close();
    }

    if (index_.is_open()) {
        index_.flush();
        index_.close();
    }
}

bool BinarySessionWriter::isOpen() const
{
    return frames_.is_open() && index_.is_open();
}

std::uint64_t BinarySessionWriter::frameCount() const
{
    return frameCount_;
}

const std::string& BinarySessionWriter::lastError() const
{
    return lastError_;
}

bool BinarySessionWriter::writeFrameHeader()
{
    const FrameFileHeader header;
    if (!writeBinaryArray(frames_, header.magic) ||
        !writeBinaryValue(frames_, header.formatVersion) ||
        !writeBinaryValue(frames_, header.endian) ||
        !writeBinaryValue(frames_, header.headerSize) ||
        !writeBinaryValue(frames_, header.reserved)) {
        setError("failed to write frame file header");
        return false;
    }

    return true;
}

bool BinarySessionWriter::writeIndexHeader()
{
    const FrameIndexHeader header;
    if (!writeBinaryArray(index_, header.magic) ||
        !writeBinaryValue(index_, header.formatVersion) ||
        !writeBinaryValue(index_, header.entrySize)) {
        setError("failed to write frame index header");
        return false;
    }

    return true;
}

void BinarySessionWriter::setError(std::string message)
{
    lastError_ = std::move(message);
}

} // namespace ovtr
