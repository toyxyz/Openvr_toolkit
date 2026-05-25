#pragma once

#include <array>
#include <cstdint>

namespace ovtr {

inline constexpr std::array<char, 8> kFrameFileMagic{'O', 'V', 'T', 'R', 'B', 'I', 'N', '\0'};
inline constexpr std::array<char, 8> kFrameIndexMagic{'O', 'V', 'T', 'R', 'I', 'D', 'X', '\0'};
inline constexpr std::uint32_t kBinaryFormatVersion = 1;
inline constexpr std::uint32_t kEndianMarker = 0x01020304u;

struct FrameFileHeader {
    std::array<char, 8> magic = kFrameFileMagic;
    std::uint32_t formatVersion = kBinaryFormatVersion;
    std::uint32_t endian = kEndianMarker;
    std::uint32_t headerSize = 24;
    std::uint32_t reserved = 0;
};

struct FrameIndexHeader {
    std::array<char, 8> magic = kFrameIndexMagic;
    std::uint32_t formatVersion = kBinaryFormatVersion;
    std::uint32_t entrySize = 24;
};

struct FrameIndexEntry {
    std::uint64_t frameIndex = 0;
    std::uint64_t timestampNs = 0;
    std::uint64_t byteOffset = 0;
};

} // namespace ovtr

