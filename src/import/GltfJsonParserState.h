#pragma once

#include "import/GltfJson.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace ovtr::detail {

class JsonParserState {
public:
    explicit JsonParserState(const std::string& text);

    bool parse(JsonValue& value);
    const std::string& error() const;

private:
    const std::string& text_;
    std::size_t position_ = 0;
    std::string error_;

    void setError(const std::string& message);
    void skipWhitespace();
    bool consume(char expected);
    bool consumeLiteral(const char* literal);
    bool parseValue(JsonValue& value);
    bool parseObject(JsonValue& value);
    bool parseArray(JsonValue& value);
    bool parseHex4(std::uint32_t& value);
    bool parseString(std::string& output);
    bool parseNumber(double& output);
};

} // namespace ovtr::detail
