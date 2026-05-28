#include "TestSupport.h"

#include "util/BinaryBuffer.h"

#include <fstream>
#include <iterator>

namespace ovtr::test {

std::string readTextFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::vector<std::uint8_t> readBinaryFile(const std::filesystem::path& path)
{
    std::vector<std::uint8_t> bytes;
    require(ovtr::readFileBytes(path, bytes), "failed to read binary test file");
    return bytes;
}

std::uint32_t readLittleEndianUint32(const std::vector<std::uint8_t>& bytes, const std::size_t offset)
{
    std::uint32_t value = 0;
    require(ovtr::readLittleEndianUint32(bytes, offset, value), "binary file is too small for uint32");
    return value;
}

} // namespace ovtr::test
