#include "import/GltfJsonParserState.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>

namespace ovtr::detail {

bool JsonParserState::parseNumber(double& output)
{
    const char* start = text_.c_str() + position_;
    char* end = nullptr;
    errno = 0;
    output = std::strtod(start, &end);
    if (end == start || errno == ERANGE || !std::isfinite(output)) {
        setError("invalid JSON number");
        return false;
    }
    position_ = static_cast<std::size_t>(end - text_.c_str());
    return true;
}

} // namespace ovtr::detail
