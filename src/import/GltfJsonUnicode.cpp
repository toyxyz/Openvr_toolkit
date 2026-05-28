#include "import/GltfJsonUnicode.h"

namespace ovtr::detail {

int jsonHexValue(const char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return -1;
}

void appendUtf8CodePoint(std::string& output, const std::uint32_t codePoint)
{
    if (codePoint <= 0x7fu) {
        output.push_back(static_cast<char>(codePoint));
    } else if (codePoint <= 0x7ffu) {
        output.push_back(static_cast<char>(0xc0u | (codePoint >> 6u)));
        output.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
    } else if (codePoint <= 0xffffu) {
        output.push_back(static_cast<char>(0xe0u | (codePoint >> 12u)));
        output.push_back(static_cast<char>(0x80u | ((codePoint >> 6u) & 0x3fu)));
        output.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
    } else {
        output.push_back(static_cast<char>(0xf0u | (codePoint >> 18u)));
        output.push_back(static_cast<char>(0x80u | ((codePoint >> 12u) & 0x3fu)));
        output.push_back(static_cast<char>(0x80u | ((codePoint >> 6u) & 0x3fu)));
        output.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
    }
}

} // namespace ovtr::detail
