#include "import/GltfJson.h"

namespace ovtr {

const JsonValue* JsonValue::find(const char* key) const
{
    if (type != Type::Object || key == nullptr) {
        return nullptr;
    }
    for (const auto& entry : objectValue) {
        if (entry.first == key) {
            return &entry.second;
        }
    }
    return nullptr;
}

} // namespace ovtr
