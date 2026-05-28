#include "util/BinaryBuffer.h"

#include <cstring>
#include <fstream>
#include <iterator>

namespace ovtr {

void padToAlignment(std::vector<std::uint8_t>& bytes, const std::size_t alignment, const std::uint8_t padding)
{
    if (alignment == 0) {
        return;
    }

    while ((bytes.size() % alignment) != 0) {
        bytes.push_back(padding);
    }
}

void appendLittleEndianUint16(std::vector<std::uint8_t>& bytes, const std::uint16_t value)
{
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
}

void appendLittleEndianUint32(std::vector<std::uint8_t>& bytes, const std::uint32_t value)
{
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16u) & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24u) & 0xffu));
}

void appendLittleEndianFloat32(std::vector<std::uint8_t>& bytes, const float value)
{
    static_assert(sizeof(float) == sizeof(std::uint32_t));
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    appendLittleEndianUint32(bytes, bits);
}

bool readLittleEndianUint32(const std::vector<std::uint8_t>& bytes, const std::size_t offset, std::uint32_t& value)
{
    if (offset + sizeof(std::uint32_t) > bytes.size()) {
        value = 0;
        return false;
    }

    value = static_cast<std::uint32_t>(bytes[offset]) |
        (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u) |
        (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u) |
        (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u);
    return true;
}

std::vector<std::uint8_t> paddedStringBytes(const std::string& text, const char padding)
{
    std::vector<std::uint8_t> bytes(text.begin(), text.end());
    padToAlignment(bytes, 4, static_cast<std::uint8_t>(padding));
    return bytes;
}

std::vector<std::uint8_t> paddedBinaryBytes(const std::vector<std::uint8_t>& input)
{
    std::vector<std::uint8_t> bytes = input;
    padToAlignment(bytes);
    return bytes;
}

bool ensureParentDirectory(const std::filesystem::path& path, std::string* error)
{
    const std::filesystem::path parentPath = path.parent_path();
    if (parentPath.empty()) {
        return true;
    }

    std::error_code createError;
    std::filesystem::create_directories(parentPath, createError);
    if (createError) {
        if (error != nullptr) {
            *error = createError.message();
        }
        return false;
    }
    return true;
}

bool readFileBytes(const std::filesystem::path& path, std::vector<std::uint8_t>& bytes, std::string* error)
{
    bytes.clear();

    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        if (error != nullptr) {
            *error = "failed to open binary file";
        }
        return false;
    }

    bytes.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    if (!input.good() && !input.eof()) {
        if (error != nullptr) {
            *error = "failed to read binary file";
        }
        bytes.clear();
        return false;
    }

    return true;
}

bool writeFileBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes, std::string* error)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        if (error != nullptr) {
            *error = "failed to open binary output file";
        }
        return false;
    }

    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!output.good()) {
        if (error != nullptr) {
            *error = "failed to write binary output file";
        }
        return false;
    }

    return true;
}

} // namespace ovtr
