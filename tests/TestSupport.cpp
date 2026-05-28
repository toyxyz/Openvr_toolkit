#include "TestSupport.h"

#include <stdexcept>

namespace ovtr::test {

void require(const bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

ScopedCurrentPath::ScopedCurrentPath(const std::filesystem::path& path)
    : previous_(std::filesystem::current_path())
{
    std::filesystem::current_path(path);
}

ScopedCurrentPath::~ScopedCurrentPath()
{
    std::error_code ignored;
    std::filesystem::current_path(previous_, ignored);
}

} // namespace ovtr::test
