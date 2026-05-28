#include "export/GltfJsonFormatting.h"

#include <iomanip>
#include <ostream>
#include <sstream>

namespace ovtr::detail {

std::string formatGltfJsonDouble(const double value)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(9) << value;
    return stream.str();
}

void writeGltfJsonNumberArray(std::ostream& out, const std::vector<double>& values)
{
    out << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            out << ",";
        }
        out << formatGltfJsonDouble(values[i]);
    }
    out << "]";
}

void writeGltfJsonFloatArray(std::ostream& out, const std::array<float, 3>& values)
{
    out << "[" << formatGltfJsonDouble(values[0]) << "," << formatGltfJsonDouble(values[1]) << ","
        << formatGltfJsonDouble(values[2]) << "]";
}

void writeGltfJsonFloatArray(std::ostream& out, const std::array<float, 4>& values)
{
    out << "[" << formatGltfJsonDouble(values[0]) << "," << formatGltfJsonDouble(values[1]) << ","
        << formatGltfJsonDouble(values[2]) << "," << formatGltfJsonDouble(values[3]) << "]";
}

} // namespace ovtr::detail
