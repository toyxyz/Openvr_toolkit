#include "util/JsonWriter.h"

#include <iomanip>
#include <ostream>
#include <sstream>

namespace ovtr {

std::string escapeJsonString(const std::string& input)
{
    std::ostringstream stream;
    for (const unsigned char ch : input) {
        switch (ch) {
        case '\\':
            stream << "\\\\";
            break;
        case '"':
            stream << "\\\"";
            break;
        case '\b':
            stream << "\\b";
            break;
        case '\f':
            stream << "\\f";
            break;
        case '\n':
            stream << "\\n";
            break;
        case '\r':
            stream << "\\r";
            break;
        case '\t':
            stream << "\\t";
            break;
        default:
            if (ch < 0x20) {
                stream << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                       << static_cast<int>(ch) << std::dec << std::setfill(' ');
            } else {
                stream << static_cast<char>(ch);
            }
            break;
        }
    }
    return stream.str();
}

void writeJsonString(std::ostream& output, const std::string& input)
{
    output << '"' << escapeJsonString(input) << '"';
}

} // namespace ovtr
