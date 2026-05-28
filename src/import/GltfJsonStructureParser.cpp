#include "import/GltfJsonParserState.h"

#include <utility>

namespace ovtr::detail {

bool JsonParserState::parseObject(JsonValue& value)
{
    if (!consume('{')) {
        return false;
    }
    value.type = JsonValue::Type::Object;
    value.objectValue.clear();

    skipWhitespace();
    if (position_ < text_.size() && text_[position_] == '}') {
        ++position_;
        return true;
    }

    while (true) {
        skipWhitespace();
        if (position_ >= text_.size() || text_[position_] != '"') {
            setError("expected JSON object key");
            return false;
        }

        std::string key;
        if (!parseString(key)) {
            return false;
        }

        skipWhitespace();
        if (!consume(':')) {
            return false;
        }

        JsonValue child;
        if (!parseValue(child)) {
            return false;
        }
        value.objectValue.emplace_back(std::move(key), std::move(child));

        skipWhitespace();
        if (position_ < text_.size() && text_[position_] == ',') {
            ++position_;
            continue;
        }
        if (position_ < text_.size() && text_[position_] == '}') {
            ++position_;
            return true;
        }

        setError("expected ',' or '}'");
        return false;
    }
}

bool JsonParserState::parseArray(JsonValue& value)
{
    if (!consume('[')) {
        return false;
    }
    value.type = JsonValue::Type::Array;
    value.arrayValue.clear();

    skipWhitespace();
    if (position_ < text_.size() && text_[position_] == ']') {
        ++position_;
        return true;
    }

    while (true) {
        JsonValue child;
        if (!parseValue(child)) {
            return false;
        }
        value.arrayValue.push_back(std::move(child));

        skipWhitespace();
        if (position_ < text_.size() && text_[position_] == ',') {
            ++position_;
            continue;
        }
        if (position_ < text_.size() && text_[position_] == ']') {
            ++position_;
            return true;
        }

        setError("expected ',' or ']'");
        return false;
    }
}

} // namespace ovtr::detail
