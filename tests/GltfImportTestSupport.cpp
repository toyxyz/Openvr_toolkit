#include "GltfImportTestSupport.h"

#include "TestSupport.h"

#include "util/BinaryBuffer.h"

#include <cstdint>
#include <vector>

namespace ovtr::test {
namespace {

constexpr std::uint32_t kGlbMagic = 0x46546c67u;
constexpr std::uint32_t kGlbVersion = 2u;
constexpr std::uint32_t kGlbJsonChunkType = 0x4e4f534au;
constexpr std::uint32_t kGlbBinChunkType = 0x004e4942u;

void appendChunk(
    std::vector<std::uint8_t>& output,
    const std::uint32_t chunkType,
    const std::vector<std::uint8_t>& chunkBytes
)
{
    ovtr::appendLittleEndianUint32(output, static_cast<std::uint32_t>(chunkBytes.size()));
    ovtr::appendLittleEndianUint32(output, chunkType);
    output.insert(output.end(), chunkBytes.begin(), chunkBytes.end());
}

} // namespace

void writeTestGlb(
    const std::filesystem::path& path,
    const std::string& json,
    const std::vector<std::uint8_t>& binary,
    const bool includeJson,
    const bool includeBinary
)
{
    std::vector<std::uint8_t> chunks;
    if (includeJson) {
        appendChunk(chunks, kGlbJsonChunkType, ovtr::paddedStringBytes(json, ' '));
    }
    if (includeBinary) {
        appendChunk(chunks, kGlbBinChunkType, ovtr::paddedBinaryBytes(binary));
    }

    std::vector<std::uint8_t> glb;
    ovtr::appendLittleEndianUint32(glb, kGlbMagic);
    ovtr::appendLittleEndianUint32(glb, kGlbVersion);
    ovtr::appendLittleEndianUint32(glb, static_cast<std::uint32_t>(12 + chunks.size()));
    glb.insert(glb.end(), chunks.begin(), chunks.end());

    require(ovtr::writeFileBytes(path, glb), "failed to write importer test GLB");
}

std::string minimalGltfJson(const std::string& accessors, const std::string& meshes)
{
    return std::string("{")
        + "\"asset\":{\"version\":\"2.0\"},"
        + "\"buffers\":[{\"byteLength\":16}],"
        + "\"bufferViews\":["
        + "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":12},"
        + "{\"buffer\":0,\"byteOffset\":12,\"byteLength\":4}"
        + "],"
        + "\"accessors\":" + accessors + ","
        + "\"nodes\":[{\"name\":\"Tracker\",\"mesh\":0}],"
        + "\"meshes\":" + meshes
        + "}";
}

} // namespace ovtr::test
