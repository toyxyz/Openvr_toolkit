#pragma once

#include <string>
#include <utility>
#include <vector>

namespace ovtr {

struct JsonValue {
    enum class Type {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object,
    };

    Type type = Type::Null;
    bool boolValue = false;
    double numberValue = 0.0;
    std::string stringValue;
    std::vector<JsonValue> arrayValue;
    std::vector<std::pair<std::string, JsonValue>> objectValue;

    const JsonValue* find(const char* key) const;
};

class JsonParser {
public:
    explicit JsonParser(const std::string& text);

    bool parse(JsonValue& value);
    const std::string& error() const;

private:
    const std::string& text_;
    std::string error_;
};

} // namespace ovtr
