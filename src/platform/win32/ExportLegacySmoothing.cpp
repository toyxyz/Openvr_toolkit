#include "platform/win32/ExportLegacySmoothing.h"

#include "platform/win32/ConfigStore.h"

#include <algorithm>

namespace ovtr::win32 {
namespace {

void smoothAxisPass(std::vector<float>& axis)
{
    const std::vector<float> source = axis;
    const int count = static_cast<int>(source.size());
    for (int index = 1; index < count - 1; ++index) {
        const int p1 = index - 1;
        const int p2 = (index - 2 > 0) ? index - 2 : p1;
        const int n1 = index + 1;
        const int n2 = std::min(count - 1, index + 2);
        axis[static_cast<std::size_t>(index)] =
            (2.0f * source[static_cast<std::size_t>(p2)] +
             3.0f * source[static_cast<std::size_t>(p1)] +
             2.0f * source[static_cast<std::size_t>(index)] +
             3.0f * source[static_cast<std::size_t>(n1)] +
             2.0f * source[static_cast<std::size_t>(n2)]) /
            12.0f;
    }
}

} // namespace

ExportLegacySmoothingReport smoothExportPositionsLegacy(
    std::array<std::vector<float>, 3>& axes,
    const int iterations
) {
    ExportLegacySmoothingReport report;
    const int passCount = sanitizedSmoothingIterations(iterations);
    if (passCount <= 0 || axes[0].size() < 3) {
        return report;
    }

    for (int iteration = 0; iteration < passCount; ++iteration) {
        for (std::vector<float>& axis : axes) {
            smoothAxisPass(axis);
        }
    }
    report.smoothedSamples = axes[0].size();
    return report;
}

} // namespace ovtr::win32
