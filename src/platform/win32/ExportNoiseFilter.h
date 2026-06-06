#pragma once

#include "data/SessionTypes.h"
#include "platform/win32/ConfigTypes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ovtr::win32 {

struct ExportNoiseFilterSettings {
    bool enabled = false;
    float cutoffHz = 8.0f;
    OutlierRepairStrength outlierRepairStrength = OutlierRepairStrength::Light;
    int smoothingIterations = 0;
};

struct ExportNoiseFilterReport {
    std::size_t filteredSamples = 0;
    std::size_t skippedShortRuns = 0;
    std::size_t invalidOutputs = 0;
    std::size_t outlierCandidates = 0;
    std::size_t repairedOutliers = 0;
    std::size_t skippedOutlierRuns = 0;
    std::size_t invalidRepairOutputs = 0;
    std::size_t smoothedSamples = 0;
    double maxObservedStepMeters = 0.0;
    double outlierStepThresholdMeters = 0.0;
};

struct ExportNoiseFilterRunDiagnostic {
    std::uint32_t runtimeIndex = 0;
    std::size_t startFrame = 0;
    std::size_t endFrame = 0;
    std::size_t sampleCount = 0;
    std::size_t skippedShortSamples = 0;
    std::size_t outlierCandidates = 0;
    std::size_t repairedOutliers = 0;
    std::size_t skippedOutlierRuns = 0;
    std::size_t invalidRepairOutputs = 0;
    std::size_t smoothedSamples = 0;
    double maxObservedStepMeters = 0.0;
    double outlierStepThresholdMeters = 0.0;
};

struct ExportNoiseFilterResult {
    ExportNoiseFilterReport report;
    std::vector<ExportNoiseFilterRunDiagnostic> runDiagnostics;
};

ExportNoiseFilterResult applyExportNoiseFilterToFrames(
    std::vector<ovtr::FrameSample>& frames,
    double sampleRate,
    ExportNoiseFilterSettings settings
);

} // namespace ovtr::win32
