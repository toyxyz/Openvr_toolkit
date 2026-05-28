#include "export/FbxAsciiFormatting.h"

#include <iomanip>
#include <sstream>

namespace ovtr {

std::string escapeFbxString(const std::string& input)
{
    std::string output;
    output.reserve(input.size());
    for (const char ch : input) {
        output.push_back(ch == '"' ? '_' : ch);
    }
    return output;
}

std::string joinedDoubles(const std::vector<double>& values)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(9);
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            stream << ",";
        }
        stream << values[i];
    }
    return stream.str();
}

std::string joinedInt64(const std::vector<std::int64_t>& values)
{
    std::ostringstream stream;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            stream << ",";
        }
        stream << values[i];
    }
    return stream.str();
}

std::string joinedInt32(const std::vector<std::int32_t>& values)
{
    std::ostringstream stream;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            stream << ",";
        }
        stream << values[i];
    }
    return stream.str();
}

} // namespace ovtr
