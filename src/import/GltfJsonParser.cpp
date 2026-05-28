#include "import/GltfJson.h"

#include "import/GltfJsonParserState.h"

namespace ovtr {

JsonParser::JsonParser(const std::string& text)
    : text_(text)
{
}

bool JsonParser::parse(JsonValue& value)
{
    detail::JsonParserState state(text_);
    if (!state.parse(value)) {
        error_ = state.error();
        return false;
    }
    error_.clear();
    return true;
}

const std::string& JsonParser::error() const
{
    return error_;
}

} // namespace ovtr
