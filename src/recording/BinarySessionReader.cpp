#include "recording/BinarySessionReader.h"

#include "recording/BinaryIO.h"

#include <algorithm>
#include <utility>

namespace ovtr {

BinarySessionReader::~BinarySessionReader()
{
    close();
}

bool BinarySessionReader::open(const std::filesystem::path& framesPath, const std::filesystem::path& indexPath)
{
    close();
    lastError_.clear();

    frames_.open(framesPath, std::ios::binary);
    if (!frames_.is_open()) {
        setError("failed to open frame file");
        return false;
    }

    if (!readFrameHeader()) {
        close();
        return false;
    }

    std::ifstream indexFile(indexPath, std::ios::binary);
    if (!indexFile.is_open()) {
        setError("failed to open index file");
        close();
        return false;
    }

    std::array<char, 8> magic{};
    std::uint32_t formatVersion = 0;
    std::uint32_t entrySize = 0;
    if (!readBinaryArray(indexFile, magic) ||
        !readBinaryValue(indexFile, formatVersion) ||
        !readBinaryValue(indexFile, entrySize)) {
        setError("failed to read index header");
        close();
        return false;
    }

    if (magic != kFrameIndexMagic || formatVersion != kBinaryFormatVersion || entrySize != 24) {
        setError("unsupported frame index format");
        close();
        return false;
    }

    index_.clear();
    while (true) {
        FrameIndexEntry entry;
        if (!readBinaryValue(indexFile, entry.frameIndex) ||
            !readBinaryValue(indexFile, entry.timestampNs) ||
            !readBinaryValue(indexFile, entry.byteOffset)) {
            if (indexFile.eof()) {
                break;
            }
            setError("failed to read frame index entry");
            close();
            return false;
        }
        index_.push_back(entry);
    }

    return true;
}

void BinarySessionReader::close()
{
    if (frames_.is_open()) {
        frames_.close();
    }
    index_.clear();
}

bool BinarySessionReader::readFrame(const std::uint64_t frameIndex, FrameSample& outFrame)
{
    const auto found = std::lower_bound(
        index_.begin(),
        index_.end(),
        frameIndex,
        [](const FrameIndexEntry& entry, const std::uint64_t value) {
            return entry.frameIndex < value;
        }
    );

    if (found == index_.end() || found->frameIndex != frameIndex) {
        setError("frame index not found");
        return false;
    }

    return readFrameAtOffset(found->byteOffset, outFrame);
}

std::uint64_t BinarySessionReader::frameCount() const
{
    return static_cast<std::uint64_t>(index_.size());
}

const std::string& BinarySessionReader::lastError() const
{
    return lastError_;
}

bool BinarySessionReader::readFrameHeader()
{
    std::array<char, 8> magic{};
    std::uint32_t formatVersion = 0;
    std::uint32_t endian = 0;
    std::uint32_t headerSize = 0;
    std::uint32_t reserved = 0;

    if (!readBinaryArray(frames_, magic) ||
        !readBinaryValue(frames_, formatVersion) ||
        !readBinaryValue(frames_, endian) ||
        !readBinaryValue(frames_, headerSize) ||
        !readBinaryValue(frames_, reserved)) {
        setError("failed to read frame file header");
        return false;
    }

    if (magic != kFrameFileMagic ||
        formatVersion != kBinaryFormatVersion ||
        endian != kEndianMarker ||
        headerSize != 24) {
        setError("unsupported frame file format");
        return false;
    }

    return true;
}

bool BinarySessionReader::readFrameAtOffset(const std::uint64_t byteOffset, FrameSample& outFrame)
{
    if (!frames_.is_open()) {
        setError("reader is not open");
        return false;
    }

    frames_.clear();
    frames_.seekg(static_cast<std::streamoff>(byteOffset), std::ios::beg);
    if (!frames_.good()) {
        setError("failed to seek frame file");
        return false;
    }

    std::uint32_t poseCount = 0;
    std::uint32_t frameFlags = 0;
    if (!readBinaryValue(frames_, outFrame.frameIndex) ||
        !readBinaryValue(frames_, outFrame.timestampNs) ||
        !readBinaryValue(frames_, outFrame.timeSeconds) ||
        !readBinaryValue(frames_, poseCount) ||
        !readBinaryValue(frames_, frameFlags)) {
        setError("failed to read frame record header");
        return false;
    }

    outFrame.poses.clear();
    outFrame.poses.reserve(poseCount);
    for (std::uint32_t i = 0; i < poseCount; ++i) {
        PoseSample pose;
        if (!readBinaryValue(frames_, pose.deviceId) ||
            !readBinaryValue(frames_, pose.runtimeIndex) ||
            !readBinaryArray(frames_, pose.position) ||
            !readBinaryArray(frames_, pose.rotation) ||
            !readBinaryArray(frames_, pose.velocity) ||
            !readBinaryArray(frames_, pose.angularVelocity) ||
            !readBinaryValue(frames_, pose.flags)) {
            setError("failed to read pose record");
            return false;
        }
        outFrame.poses.push_back(pose);
    }

    return true;
}

void BinarySessionReader::setError(std::string message)
{
    lastError_ = std::move(message);
}

} // namespace ovtr
