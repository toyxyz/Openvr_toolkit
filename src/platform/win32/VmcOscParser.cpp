#include "platform/win32/VmcOscParser.h"

#include <cstring>

namespace ovtr::win32 {
namespace {

constexpr const char* kBonePosAddress = "/VMC/Ext/Bone/Pos";
constexpr const char* kBundleHeader = "#bundle";

std::size_t align4(const std::size_t value) noexcept
{
    return (value + 3u) & ~std::size_t{3u};
}

bool readString(const std::uint8_t* data, const std::size_t size, std::size_t& offset, std::string& out)
{
    if (offset >= size) {
        return false;
    }
    const std::size_t start = offset;
    while (offset < size && data[offset] != 0) {
        ++offset;
    }
    if (offset >= size) {
        return false;
    }
    out.assign(reinterpret_cast<const char*>(data + start), offset - start);
    offset = align4(offset + 1u);
    return offset <= size;
}

bool readInt32(const std::uint8_t* data, const std::size_t size, std::size_t& offset, std::int32_t& out)
{
    if (offset + 4u > size) {
        return false;
    }
    out = static_cast<std::int32_t>(
        (static_cast<std::uint32_t>(data[offset]) << 24u) |
        (static_cast<std::uint32_t>(data[offset + 1u]) << 16u) |
        (static_cast<std::uint32_t>(data[offset + 2u]) << 8u) |
        static_cast<std::uint32_t>(data[offset + 3u])
    );
    offset += 4u;
    return true;
}

bool readFloat32(const std::uint8_t* data, const std::size_t size, std::size_t& offset, float& out)
{
    std::int32_t bits = 0;
    if (!readInt32(data, size, offset, bits)) {
        return false;
    }
    static_assert(sizeof(float) == sizeof(std::uint32_t));
    const std::uint32_t unsignedBits = static_cast<std::uint32_t>(bits);
    std::memcpy(&out, &unsignedBits, sizeof(float));
    return true;
}

bool parseMessage(const std::uint8_t* data, const std::size_t size, std::vector<VmcOscBonePose>& outBones)
{
    std::size_t offset = 0;
    std::string address;
    std::string tags;
    if (!readString(data, size, offset, address) || !readString(data, size, offset, tags)) {
        return false;
    }
    if (address != kBonePosAddress || tags != ",sfffffff") {
        return true;
    }

    VmcOscBonePose bone;
    if (!readString(data, size, offset, bone.name)) {
        return true;
    }
    for (float& value : bone.position) {
        if (!readFloat32(data, size, offset, value)) {
            return true;
        }
    }
    for (float& value : bone.rotation) {
        if (!readFloat32(data, size, offset, value)) {
            return true;
        }
    }
    outBones.push_back(bone);
    return true;
}

bool parseBundle(const std::uint8_t* data, const std::size_t size, std::vector<VmcOscBonePose>& outBones)
{
    std::size_t offset = 16u;
    while (offset + 4u <= size) {
        std::int32_t elementSize = 0;
        if (!readInt32(data, size, offset, elementSize) || elementSize <= 0) {
            return false;
        }
        const std::size_t count = static_cast<std::size_t>(elementSize);
        if (offset + count > size) {
            return false;
        }
        parseVmcOscPacket(data + offset, count, outBones);
        offset += count;
    }
    return true;
}

} // namespace

bool parseVmcOscPacket(
    const std::uint8_t* data,
    const std::size_t size,
    std::vector<VmcOscBonePose>& outBones
) {
    if (data == nullptr || size < 4u) {
        return false;
    }
    if (size >= 16u && std::memcmp(data, kBundleHeader, 7u) == 0) {
        return parseBundle(data, size, outBones);
    }
    return parseMessage(data, size, outBones);
}

} // namespace ovtr::win32
