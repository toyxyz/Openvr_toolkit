#include "import/GltfJsonUtils.h"

namespace ovtr {

bool jsonNumber(const JsonValue& value, double& output)
{
    if (value.type != JsonValue::Type::Number) {
        return false;
    }
    output = value.numberValue;
    return true;
}

int jsonIntMember(const JsonValue& object, const char* key, const int defaultValue)
{
    const JsonValue* value = object.find(key);
    if (!value || value->type != JsonValue::Type::Number) {
        return defaultValue;
    }
    return static_cast<int>(value->numberValue);
}

std::size_t jsonSizeMember(
    const JsonValue& object,
    const char* key,
    const std::size_t defaultValue
)
{
    const JsonValue* value = object.find(key);
    if (!value || value->type != JsonValue::Type::Number || value->numberValue < 0.0) {
        return defaultValue;
    }
    return static_cast<std::size_t>(value->numberValue);
}

std::string jsonStringMember(const JsonValue& object, const char* key, std::string defaultValue)
{
    const JsonValue* value = object.find(key);
    if (!value || value->type != JsonValue::Type::String) {
        return defaultValue;
    }
    return value->stringValue;
}

bool readJsonFloatArray(
    const JsonValue& object,
    const char* key,
    float* output,
    const std::size_t expectedCount
)
{
    const JsonValue* value = object.find(key);
    if (!value || value->type != JsonValue::Type::Array || value->arrayValue.size() < expectedCount) {
        return false;
    }

    for (std::size_t i = 0; i < expectedCount; ++i) {
        double number = 0.0;
        if (!jsonNumber(value->arrayValue[i], number)) {
            return false;
        }
        output[i] = static_cast<float>(number);
    }
    return true;
}

} // namespace ovtr
