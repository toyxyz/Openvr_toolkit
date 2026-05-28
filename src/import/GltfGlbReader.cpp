#include "import/GltfGlbReader.h"

#include "util/BinaryBuffer.h"

#include <cstddef>

namespace ovtr {
namespace {

constexpr std::uint32_t kGlbMagic = 0x46546c67u;
constexpr std::uint32_t kGlbVersion = 2u;
constexpr std::uint32_t kGlbJsonChunkType = 0x4e4f534au;
constexpr std::uint32_t kGlbBinChunkType = 0x004e4942u;

} // namespace

bool readGltfGlbPayload(
    const std::filesystem::path& path,
    GltfGlbPayload& payload,
    std::string& error
)
{
    payload = {};

    std::vector<std::uint8_t> bytes;
    if (!readFileBytes(path, bytes) || bytes.empty()) {
        error = "could not read GLB file";
        return false;
    }
    if (bytes.size() < 20) {
        error = "GLB file is too small";
        return false;
    }
    std::uint32_t value = 0;
    if (!readLittleEndianUint32(bytes, 0, value) || value != kGlbMagic) {
        error = "file is not a GLB";
        return false;
    }
    if (!readLittleEndianUint32(bytes, 4, value) || value != kGlbVersion) {
        error = "unsupported GLB version";
        return false;
    }

    std::uint32_t declaredLength = 0;
    if (!readLittleEndianUint32(bytes, 8, declaredLength)) {
        error = "GLB file is too small";
        return false;
    }
    if (declaredLength > bytes.size()) {
        error = "GLB declared length exceeds file size";
        return false;
    }
    if (declaredLength != bytes.size()) {
        error = "GLB declared length does not match file size";
        return false;
    }

    std::size_t offset = 12;
    while (offset < declaredLength) {
        if (declaredLength - offset < 8) {
            error = "GLB chunk header exceeds declared length";
            return false;
        }

        std::uint32_t chunkLength = 0;
        std::uint32_t chunkType = 0;
        if (!readLittleEndianUint32(bytes, offset, chunkLength) ||
            !readLittleEndianUint32(bytes, offset + 4, chunkType)) {
            error = "GLB chunk header exceeds file size";
            return false;
        }
        offset += 8;
        if (chunkLength > declaredLength - offset) {
            error = "GLB chunk length exceeds declared length";
            return false;
        }

        if (chunkType == kGlbJsonChunkType) {
            payload.json.assign(
                reinterpret_cast<const char*>(bytes.data() + offset),
                reinterpret_cast<const char*>(bytes.data() + offset + chunkLength)
            );
        } else if (chunkType == kGlbBinChunkType) {
            payload.binary.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                bytes.begin() + static_cast<std::ptrdiff_t>(offset + chunkLength));
        }

        offset += chunkLength;
    }

    if (payload.json.empty()) {
        error = "GLB does not contain a JSON chunk";
        return false;
    }
    if (payload.binary.empty()) {
        error = "GLB does not contain an embedded binary chunk";
        return false;
    }
    return true;
}

} // namespace ovtr
