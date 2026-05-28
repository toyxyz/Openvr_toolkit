#include "export/GltfFileWriter.h"

#include "util/BinaryBuffer.h"

#include <fstream>

namespace ovtr {

bool writeGltfTextFile(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out << text;
    return out.good();
}

bool writeGltfBinaryFile(const std::filesystem::path& path, const std::vector<std::uint8_t>& binary)
{
    return writeFileBytes(path, binary);
}

bool writeGltfGlbFile(
    const std::filesystem::path& path,
    const std::string& json,
    const std::vector<std::uint8_t>& binary
)
{
    constexpr std::uint32_t kGlbMagic = 0x46546c67u;
    constexpr std::uint32_t kGlbVersion = 2u;
    constexpr std::uint32_t kJsonChunkType = 0x4e4f534au;
    constexpr std::uint32_t kBinChunkType = 0x004e4942u;

    const std::vector<std::uint8_t> jsonChunk = paddedStringBytes(json, ' ');
    const std::vector<std::uint8_t> binChunk = paddedBinaryBytes(binary);
    const std::uint32_t totalLength = static_cast<std::uint32_t>(12 + 8 + jsonChunk.size() + 8 + binChunk.size());

    std::vector<std::uint8_t> output;
    output.reserve(totalLength);
    appendLittleEndianUint32(output, kGlbMagic);
    appendLittleEndianUint32(output, kGlbVersion);
    appendLittleEndianUint32(output, totalLength);
    appendLittleEndianUint32(output, static_cast<std::uint32_t>(jsonChunk.size()));
    appendLittleEndianUint32(output, kJsonChunkType);
    output.insert(output.end(), jsonChunk.begin(), jsonChunk.end());
    appendLittleEndianUint32(output, static_cast<std::uint32_t>(binChunk.size()));
    appendLittleEndianUint32(output, kBinChunkType);
    output.insert(output.end(), binChunk.begin(), binChunk.end());

    return writeFileBytes(path, output);
}

std::filesystem::path gltfSiblingBinPath(const std::filesystem::path& gltfPath)
{
    std::filesystem::path binPath = gltfPath;
    binPath.replace_extension(".bin");
    return binPath;
}

} // namespace ovtr
