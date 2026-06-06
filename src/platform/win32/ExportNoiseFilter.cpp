#include "platform/win32/ExportNoiseFilter.h"

#include "data/SkeletalSyntheticPose.h"
#include "platform/win32/AppConfig.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/ExportLegacySmoothing.h"
#include "platform/win32/ExportOutlierRepair.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace ovtr::win32 {
namespace {

constexpr std::size_t kMinFilterRunLength = 4;

struct PoseRef {
    std::size_t frame = 0;
    std::size_t pose = 0;
};

struct Biquad {
    double b0 = 0.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
};

bool finitePosition(const ovtr::PoseSample& pose) noexcept
{
    return std::isfinite(pose.position[0]) &&
        std::isfinite(pose.position[1]) &&
        std::isfinite(pose.position[2]);
}

bool filterablePose(const ovtr::PoseSample& pose) noexcept
{
    return (pose.flags & ovtr::PoseFlagPoseValid) != 0 &&
        !ovtr::isSkeletalBoneRuntimeIndex(pose.runtimeIndex) &&
        finitePosition(pose);
}

Biquad butterworthLowPass(const double sampleRate, const float cutoffHz) noexcept
{
    const double safeRate = sampleRate > 0.0 ? sampleRate : kDefaultRecordExportSampleRate;
    const double safeCutoff = std::min<double>(sanitizedNoiseFilterCutoffHz(cutoffHz), safeRate * 0.45);
    const double omega = 6.283185307179586 * safeCutoff / safeRate;
    const double sinOmega = std::sin(omega);
    const double cosOmega = std::cos(omega);
    const double alpha = sinOmega / 1.4142135623730951;
    const double a0 = 1.0 + alpha;
    return {
        ((1.0 - cosOmega) * 0.5) / a0,
        (1.0 - cosOmega) / a0,
        ((1.0 - cosOmega) * 0.5) / a0,
        (-2.0 * cosOmega) / a0,
        (1.0 - alpha) / a0
    };
}

void filterForward(const Biquad& q, float* values, const std::size_t count) noexcept
{
    double x1 = values[0];
    double x2 = values[0];
    double y1 = values[0];
    double y2 = values[0];
    for (std::size_t index = 0; index < count; ++index) {
        const double x0 = values[index];
        const double y0 = q.b0 * x0 + q.b1 * x1 + q.b2 * x2 - q.a1 * y1 - q.a2 * y2;
        values[index] = static_cast<float>(y0);
        x2 = x1;
        x1 = x0;
        y2 = y1;
        y1 = y0;
    }
}

void filterZeroPhase(const Biquad& q, float* values, const std::size_t count) noexcept
{
    filterForward(q, values, count);
    for (std::size_t left = 0, right = count - 1; left < right; ++left, --right) {
        std::swap(values[left], values[right]);
    }
    filterForward(q, values, count);
    for (std::size_t left = 0, right = count - 1; left < right; ++left, --right) {
        std::swap(values[left], values[right]);
    }
}

void applyRun(
    std::vector<ovtr::FrameSample>& frames,
    const Biquad& q,
    const std::vector<PoseRef>& run,
    const std::uint32_t runtimeIndex,
    const double sampleRate,
    const OutlierRepairStrength outlierRepairStrength,
    const int smoothingIterations,
    ExportNoiseFilterResult& result
) {
    ExportNoiseFilterRunDiagnostic diagnostic;
    diagnostic.runtimeIndex = runtimeIndex;
    diagnostic.sampleCount = run.size();
    if (!run.empty()) {
        diagnostic.startFrame = run.front().frame;
        diagnostic.endFrame = run.back().frame;
    }
    if (run.size() < kMinFilterRunLength) {
        result.report.skippedShortRuns += run.size();
        diagnostic.skippedShortSamples = run.size();
        if (!run.empty()) {
            result.runDiagnostics.push_back(diagnostic);
        }
        return;
    }

    std::array<std::vector<float>, 3> axes;
    for (std::vector<float>& axis : axes) {
        axis.resize(run.size());
    }
    for (std::size_t index = 0; index < run.size(); ++index) {
        const ovtr::PoseSample& pose = frames[run[index].frame].poses[run[index].pose];
        axes[0][index] = pose.position[0];
        axes[1][index] = pose.position[1];
        axes[2][index] = pose.position[2];
    }
    const ExportOutlierRepairReport repairReport =
        repairExportPositionOutliers(axes, sampleRate, outlierRepairStrength);
    result.report.outlierCandidates += repairReport.outlierCandidates;
    result.report.repairedOutliers += repairReport.repairedOutliers;
    result.report.skippedOutlierRuns += repairReport.skippedOutlierRuns;
    result.report.invalidRepairOutputs += repairReport.invalidRepairOutputs;
    result.report.maxObservedStepMeters =
        std::max(result.report.maxObservedStepMeters, repairReport.maxObservedStepMeters);
    result.report.outlierStepThresholdMeters =
        std::max(result.report.outlierStepThresholdMeters, repairReport.stepThresholdMeters);
    diagnostic.outlierCandidates = repairReport.outlierCandidates;
    diagnostic.repairedOutliers = repairReport.repairedOutliers;
    diagnostic.skippedOutlierRuns = repairReport.skippedOutlierRuns;
    diagnostic.invalidRepairOutputs = repairReport.invalidRepairOutputs;
    diagnostic.maxObservedStepMeters = repairReport.maxObservedStepMeters;
    diagnostic.outlierStepThresholdMeters = repairReport.stepThresholdMeters;

    for (std::vector<float>& axis : axes) {
        filterZeroPhase(q, axis.data(), axis.size());
    }
    const ExportLegacySmoothingReport smoothingReport =
        smoothExportPositionsLegacy(axes, smoothingIterations);
    result.report.smoothedSamples += smoothingReport.smoothedSamples;
    diagnostic.smoothedSamples = smoothingReport.smoothedSamples;
    result.runDiagnostics.push_back(diagnostic);

    for (std::size_t index = 0; index < run.size(); ++index) {
        if (!std::isfinite(axes[0][index]) || !std::isfinite(axes[1][index]) || !std::isfinite(axes[2][index])) {
            ++result.report.invalidOutputs;
            continue;
        }
        ovtr::PoseSample& pose = frames[run[index].frame].poses[run[index].pose];
        pose.position = {axes[0][index], axes[1][index], axes[2][index]};
        ++result.report.filteredSamples;
    }
}

int poseIndexForRuntime(const ovtr::FrameSample& frame, const std::uint32_t runtimeIndex) noexcept
{
    for (std::size_t index = 0; index < frame.poses.size(); ++index) {
        if (frame.poses[index].runtimeIndex == runtimeIndex) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

} // namespace

ExportNoiseFilterResult applyExportNoiseFilterToFrames(
    std::vector<ovtr::FrameSample>& frames,
    const double sampleRate,
    const ExportNoiseFilterSettings settings
) {
    ExportNoiseFilterResult result;
    if (!settings.enabled || frames.empty()) {
        return result;
    }

    std::unordered_set<std::uint32_t> runtimeIndices;
    for (const ovtr::FrameSample& frame : frames) {
        for (const ovtr::PoseSample& pose : frame.poses) {
            if (!ovtr::isSkeletalBoneRuntimeIndex(pose.runtimeIndex)) {
                runtimeIndices.insert(pose.runtimeIndex);
            }
        }
    }

    const Biquad q = butterworthLowPass(sampleRate, settings.cutoffHz);
    std::vector<PoseRef> run;
    for (const std::uint32_t runtimeIndex : runtimeIndices) {
        run.clear();
        for (std::size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex) {
            const int poseIndex = poseIndexForRuntime(frames[frameIndex], runtimeIndex);
            if (poseIndex >= 0 && filterablePose(frames[frameIndex].poses[static_cast<std::size_t>(poseIndex)])) {
                run.push_back({frameIndex, static_cast<std::size_t>(poseIndex)});
                continue;
            }
            applyRun(
                frames,
                q,
                run,
                runtimeIndex,
                sampleRate,
                settings.outlierRepairStrength,
                settings.smoothingIterations,
                result
            );
            run.clear();
        }
        applyRun(
            frames,
            q,
            run,
            runtimeIndex,
            sampleRate,
            settings.outlierRepairStrength,
            settings.smoothingIterations,
            result
        );
    }
    return result;
}

} // namespace ovtr::win32
