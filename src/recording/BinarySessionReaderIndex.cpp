#include "recording/BinarySessionReader.h"

#include "recording/BinaryIO.h"

namespace ovtr {

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

bool BinarySessionReader::readIndexFile(const std::filesystem::path& indexPath)
{
    std::ifstream indexFile(indexPath, std::ios::binary);
    if (!indexFile.is_open()) {
        setError("failed to open index file");
        return false;
    }

    std::array<char, 8> magic{};
    std::uint32_t formatVersion = 0;
    std::uint32_t entrySize = 0;
    if (!readBinaryArray(indexFile, magic) ||
        !readBinaryValue(indexFile, formatVersion) ||
        !readBinaryValue(indexFile, entrySize)) {
        setError("failed to read index header");
        return false;
    }

    if (magic != kFrameIndexMagic || formatVersion != kBinaryFormatVersion || entrySize != 24) {
        setError("unsupported frame index format");
        return false;
    }

    return readIndex(indexFile);
}

bool BinarySessionReader::readIndex(std::istream& indexFile)
{
    index_.clear();
    while (indexFile.peek() != std::char_traits<char>::eof()) {
        FrameIndexEntry entry;
        if (!readBinaryValue(indexFile, entry.frameIndex) ||
            !readBinaryValue(indexFile, entry.timestampNs) ||
            !readBinaryValue(indexFile, entry.byteOffset)) {
            setError("failed to read complete frame index entry");
            return false;
        }
        index_.push_back(entry);
    }

    if (indexFile.bad()) {
        setError("failed to read frame index file");
        return false;
    }
    return true;
}

} // namespace ovtr
