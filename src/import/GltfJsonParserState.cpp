#include "import/GltfJsonParserState.h"

#include <sstream>

namespace ovtr::detail {

JsonParserState::JsonParserState(const std::string& text)
    : text_(text)
{
}

bool JsonParserState::parse(JsonValue& value)
{
    skipWhitespace();
    if (!parseValue(value)) {
        return false;
    }
    skipWhitespace();
    if (position_ != text_.size()) {
        setError("unexpected trailing JSON data");
        return false;
    }
    return true;
}

const std::string& JsonParserState::error() const
{
    return error_;
}

void JsonParserState::setError(const std::string& message)
{
    if (error_.empty()) {
        std::ostringstream stream;
        stream << message << " at byte " << position_;
        error_ = stream.str();
    }
}

void JsonParserState::skipWhitespace()
{
    while (position_ < text_.size()) {
        const unsigned char ch = static_cast<unsigned char>(text_[position_]);
        if (ch != ' ' && ch != '\n' && ch != '\r' && ch != '\t') {
            break;
        }
        ++position_;
    }
}

bool JsonParserState::consume(const char expected)
{
    if (position_ >= text_.size() || text_[position_] != expected) {
        std::string message = "expected '";
        message.push_back(expected);
        message.push_back('\'');
        setError(message);
        return false;
    }
    ++position_;
    return true;
}

bool JsonParserState::consumeLiteral(const char* literal)
{
    const std::size_t start = position_;
    while (*literal != '\0') {
        if (position_ >= text_.size() || text_[position_] != *literal) {
            position_ = start;
            setError("invalid JSON literal");
            return false;
        }
        ++position_;
        ++literal;
    }
    return true;
}

} // namespace ovtr::detail
