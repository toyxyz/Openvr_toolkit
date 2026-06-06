#include "platform/win32/ExportOutlierRepair.h"

#include "platform/win32/AppConfig.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace ovtr::win32 {
namespace {

struct RepairPreset {
    int window = 0;
    double thresholdMad = 0.0;
    std::size_t maxRepairLength = 0;
    double maxJumpMeters = 0.0;
    double maxVelocityMetersPerSecond = 0.0;
};

RepairPreset presetFor(const OutlierRepairStrength strength) noexcept
{
    switch (strength) {
    case OutlierRepairStrength::Light:
        return {5, 4.0, 1, 0.30, 8.0};
    case OutlierRepairStrength::Normal:
        return {7, 3.0, 2, 0.25, 7.0};
    case OutlierRepairStrength::Strong:
        return {9, 2.5, 3, 0.20, 5.0};
    case OutlierRepairStrength::None:
    default:
        return {};
    }
}

double median(std::vector<double>& values) noexcept
{
    if (values.empty()) {
        return 0.0;
    }
    const std::size_t mid = values.size() / 2;
    std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(mid), values.end());
    const double upper = values[mid];
    if ((values.size() % 2) != 0) {
        return upper;
    }
    std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(mid - 1), values.end());
    return (upper + values[mid - 1]) * 0.5;
}

double distanceAt(const std::array<std::vector<float>, 3>& axes, const std::size_t a, const std::size_t b) noexcept
{
    double sum = 0.0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const double delta = static_cast<double>(axes[axis][a]) - static_cast<double>(axes[axis][b]);
        sum += delta * delta;
    }
    return std::sqrt(sum);
}

bool localMedianOutlier(
    const std::array<std::vector<float>, 3>& axes,
    const std::size_t index,
    const RepairPreset& preset
) {
    const int halfWindow = preset.window / 2;
    const std::size_t begin = index > static_cast<std::size_t>(halfWindow) ?
        index - static_cast<std::size_t>(halfWindow) : 0;
    const std::size_t end = std::min(axes[0].size(), index + static_cast<std::size_t>(halfWindow) + 1);
    std::vector<double> samples;
    std::vector<double> center(3, 0.0);
    for (std::size_t axis = 0; axis < 3; ++axis) {
        samples.clear();
        for (std::size_t sample = begin; sample < end; ++sample) {
            samples.push_back(static_cast<double>(axes[axis][sample]));
        }
        center[axis] = median(samples);
    }

    std::vector<double> distances;
    distances.reserve(end - begin);
    for (std::size_t sample = begin; sample < end; ++sample) {
        double sum = 0.0;
        for (std::size_t axis = 0; axis < 3; ++axis) {
            const double delta = static_cast<double>(axes[axis][sample]) - center[axis];
            sum += delta * delta;
        }
        distances.push_back(std::sqrt(sum));
    }
    std::vector<double> madValues = distances;
    const double mad = median(madValues);
    const double threshold = std::max(0.01, preset.thresholdMad * 1.4826 * mad);

    double ownSum = 0.0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const double delta = static_cast<double>(axes[axis][index]) - center[axis];
        ownSum += delta * delta;
    }
    return std::sqrt(ownSum) > threshold;
}

bool jumpOutlier(
    const std::array<std::vector<float>, 3>& axes,
    const std::size_t index,
    const double maxStep
) noexcept {
    const double prevDistance = distanceAt(axes, index, index - 1);
    const double nextDistance = distanceAt(axes, index, index + 1);
    return prevDistance > maxStep || nextDistance > maxStep;
}

void repairRange(
    std::array<std::vector<float>, 3>& axes,
    const std::size_t begin,
    const std::size_t end,
    ExportOutlierRepairReport& report
) {
    const std::size_t left = begin - 1;
    const std::size_t right = end + 1;
    const double denominator = static_cast<double>(right - left);
    for (std::size_t index = begin; index <= end; ++index) {
        const double t = static_cast<double>(index - left) / denominator;
        for (std::size_t axis = 0; axis < 3; ++axis) {
            const double value = static_cast<double>(axes[axis][left]) * (1.0 - t) +
                static_cast<double>(axes[axis][right]) * t;
            if (!std::isfinite(value)) {
                ++report.invalidRepairOutputs;
                return;
            }
            axes[axis][index] = static_cast<float>(value);
        }
    }
    report.repairedOutliers += end - begin + 1;
}

} // namespace

ExportOutlierRepairReport repairExportPositionOutliers(
    std::array<std::vector<float>, 3>& axes,
    const double sampleRate,
    const OutlierRepairStrength strength
) {
    ExportOutlierRepairReport report;
    const RepairPreset preset = presetFor(strength);
    const std::size_t count = axes[0].size();
    if (preset.maxRepairLength == 0 || count < 3) {
        return report;
    }

    const double rate = sampleRate > 0.0 ? sampleRate : kDefaultRecordExportSampleRate;
    const double maxStep = std::min(preset.maxJumpMeters, preset.maxVelocityMetersPerSecond / rate);
    report.stepThresholdMeters = maxStep;
    for (std::size_t index = 1; index < count; ++index) {
        report.maxObservedStepMeters =
            std::max(report.maxObservedStepMeters, distanceAt(axes, index, index - 1));
    }
    std::vector<bool> candidate(count, false);
    for (std::size_t index = 1; index + 1 < count; ++index) {
        candidate[index] = localMedianOutlier(axes, index, preset) && jumpOutlier(axes, index, maxStep);
        if (candidate[index]) {
            ++report.outlierCandidates;
        }
    }

    for (std::size_t index = 1; index + 1 < count;) {
        if (!candidate[index]) {
            ++index;
            continue;
        }
        const std::size_t begin = index;
        while (index + 1 < count && candidate[index]) {
            ++index;
        }
        const std::size_t end = index - 1;
        if (end - begin + 1 <= preset.maxRepairLength && begin > 0 && end + 1 < count) {
            repairRange(axes, begin, end, report);
        } else {
            ++report.skippedOutlierRuns;
        }
    }
    return report;
}

} // namespace ovtr::win32
