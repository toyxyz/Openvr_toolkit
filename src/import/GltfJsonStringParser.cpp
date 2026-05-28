#include "import/GltfJsonParserState.h"
#include "import/GltfJsonUnicode.h"

namespace ovtr::detail {

bool JsonParserState::parseHex4(std::uint32_t& value)
{
    if (position_ + 4 > text_.size()) {
        setError("incomplete JSON unicode escape");
        return false;
    }

    value = 0;
    for (int i = 0; i < 4; ++i) {
        const int digit = jsonHexValue(text_[position_ + static_cast<std::size_t>(i)]);
        if (digit < 0) {
            setError("invalid JSON unicode escape");
            return false;
        }
        value = (value << 4u) | static_cast<std::uint32_t>(digit);
    }
    position_ += 4;
    return true;
}

bool JsonParserState::parseString(std::string& output)
{
    if (!consume('"')) {
        return false;
    }

    output.clear();
    while (position_ < text_.size()) {
        const unsigned char raw = static_cast<unsigned char>(text_[position_++]);
        if (raw == '"') {
            return true;
        }
        if (raw < 0x20u) {
            setError("control character in JSON string");
            return false;
        }
        if (raw != '\\') {
            output.push_back(static_cast<char>(raw));
            continue;
        }

        if (position_ >= text_.size()) {
            setError("incomplete JSON string escape");
            return false;
        }
        const char escaped = text_[position_++];
        switch (escaped) {
        case '"':
        case '\\':
        case '/':
            output.push_back(escaped);
            break;
        case 'b':
            output.push_back('\b');
            break;
        case 'f':
            output.push_back('\f');
            break;
        case 'n':
            output.push_back('\n');
            break;
        case 'r':
            output.push_back('\r');
            break;
        case 't':
            output.push_back('\t');
            break;
        case 'u': {
            std::uint32_t codePoint = 0;
            if (!parseHex4(codePoint)) {
                return false;
            }
            appendUtf8CodePoint(output, codePoint);
            break;
        }
        default:
            setError("unsupported JSON string escape");
            return false;
        }
    }

    setError("unterminated JSON string");
    return false;
}

} // namespace ovtr::detail
