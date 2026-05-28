#include "import/GltfJsonParserState.h"

namespace ovtr::detail {

bool JsonParserState::parseValue(JsonValue& value)
{
    skipWhitespace();
    if (position_ >= text_.size()) {
        setError("unexpected end of JSON");
        return false;
    }

    const char ch = text_[position_];
    if (ch == '{') {
        return parseObject(value);
    }
    if (ch == '[') {
        return parseArray(value);
    }
    if (ch == '"') {
        value.type = JsonValue::Type::String;
        return parseString(value.stringValue);
    }
    if (ch == '-' || (ch >= '0' && ch <= '9')) {
        value.type = JsonValue::Type::Number;
        return parseNumber(value.numberValue);
    }
    if (ch == 't') {
        if (!consumeLiteral("true")) {
            return false;
        }
        value.type = JsonValue::Type::Bool;
        value.boolValue = true;
        return true;
    }
    if (ch == 'f') {
        if (!consumeLiteral("false")) {
            return false;
        }
        value.type = JsonValue::Type::Bool;
        value.boolValue = false;
        return true;
    }
    if (ch == 'n') {
        if (!consumeLiteral("null")) {
            return false;
        }
        value.type = JsonValue::Type::Null;
        return true;
    }

    setError("unexpected JSON token");
    return false;
}

} // namespace ovtr::detail
