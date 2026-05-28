#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ovtr {

void padToAlignment(std::vector<std::uint8_t>& bytes, std::size_t alignment = 4, std::uint8_t padding = 0);
void appendLittleEndianUint16(std::vector<std::uint8_t>& bytes, std::uint16_t value);
void appendLittleEndianUint32(std::vector<std::uint8_t>& bytes, std::uint32_t value);
void appendLittleEndianFloat32(std::vector<std::uint8_t>& bytes, float value);
bool readLittleEndianUint32(const std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t& value);

std::vector<std::uint8_t> paddedStringBytes(const std::string& text, char padding);
std::vector<std::uint8_t> paddedBinaryBytes(const std::vector<std::uint8_t>& input);

bool ensureParentDirectory(const std::filesystem::path& path, std::string* error = nullptr);
bool readFileBytes(const std::filesystem::path& path, std::vector<std::uint8_t>& bytes, std::string* error = nullptr);
bool writeFileBytes(
    const std::filesystem::path& path,
    const std::vector<std::uint8_t>& bytes,
    std::string* error = nullptr
);

} // namespace ovtr
