#pragma once

#include "import/GltfJson.h"

#include <cstddef>
#include <string>

namespace ovtr {

bool jsonNumber(const JsonValue& value, double& output);
int jsonIntMember(const JsonValue& object, const char* key, int defaultValue = -1);
std::size_t jsonSizeMember(const JsonValue& object, const char* key, std::size_t defaultValue = 0);
std::string jsonStringMember(const JsonValue& object, const char* key, std::string defaultValue = {});
bool readJsonFloatArray(
    const JsonValue& object,
    const char* key,
    float* output,
    std::size_t expectedCount
);

} // namespace ovtr
