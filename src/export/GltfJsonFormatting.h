#pragma once

#include <array>
#include <iosfwd>
#include <string>
#include <vector>

namespace ovtr::detail {

std::string formatGltfJsonDouble(double value);
void writeGltfJsonNumberArray(std::ostream& out, const std::vector<double>& values);
void writeGltfJsonFloatArray(std::ostream& out, const std::array<float, 3>& values);
void writeGltfJsonFloatArray(std::ostream& out, const std::array<float, 4>& values);

} // namespace ovtr::detail
