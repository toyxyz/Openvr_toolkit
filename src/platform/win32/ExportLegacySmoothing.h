#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace ovtr::win32 {

struct ExportLegacySmoothingReport {
    std::size_t smoothedSamples = 0;
};

ExportLegacySmoothingReport smoothExportPositionsLegacy(
    std::array<std::vector<float>, 3>& axes,
    int iterations
);

} // namespace ovtr::win32
