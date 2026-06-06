#pragma once

#include "platform/win32/ConfigTypes.h"

#include <array>
#include <cstddef>
#include <vector>

namespace ovtr::win32 {

struct ExportOutlierRepairReport {
    std::size_t outlierCandidates = 0;
    std::size_t repairedOutliers = 0;
    std::size_t skippedOutlierRuns = 0;
    std::size_t invalidRepairOutputs = 0;
    double maxObservedStepMeters = 0.0;
    double stepThresholdMeters = 0.0;
};

ExportOutlierRepairReport repairExportPositionOutliers(
    std::array<std::vector<float>, 3>& axes,
    double sampleRate,
    OutlierRepairStrength strength
);

} // namespace ovtr::win32
