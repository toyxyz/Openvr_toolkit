#include "TestCases.h"
#include "TestSupport.h"

#ifdef _WIN32
#include "data/SkeletalSyntheticPose.h"
#include "platform/win32/ExportLegacySmoothing.h"
#include "platform/win32/ExportNoiseFilter.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

ovtr::PoseSample physicalPose(const std::uint32_t runtimeIndex, const float x, const bool valid = true)
{
    ovtr::PoseSample pose;
    pose.deviceId = runtimeIndex;
    pose.runtimeIndex = runtimeIndex;
    pose.position = {x, 0.0f, 0.0f};
    pose.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    pose.flags = ovtr::PoseFlagDeviceConnected | ovtr::PoseFlagRecordEnabled;
    if (valid) {
        pose.flags |= ovtr::PoseFlagPoseValid;
    }
    return pose;
}

float jitterSum(const std::vector<ovtr::FrameSample>& frames, const std::uint32_t runtimeIndex, const float center)
{
    float total = 0.0f;
    for (const ovtr::FrameSample& frame : frames) {
        for (const ovtr::PoseSample& pose : frame.poses) {
            if (pose.runtimeIndex == runtimeIndex && std::isfinite(pose.position[0])) {
                total += std::fabs(pose.position[0] - center);
            }
        }
    }
    return total;
}

} // namespace

namespace ovtr::test {

void testWin32ExportNoiseFilter()
{
    std::array<std::vector<float>, 3> legacyAxes{
        std::vector<float>{0.0f, 10.0f, 20.0f, 30.0f, 40.0f},
        std::vector<float>{0.0f, 10.0f, 20.0f, 30.0f, 40.0f},
        std::vector<float>{0.0f, 10.0f, 20.0f, 30.0f, 40.0f}
    };
    const ovtr::win32::ExportLegacySmoothingReport legacyReport =
        ovtr::win32::smoothExportPositionsLegacy(legacyAxes, 1);
    require(legacyReport.smoothedSamples == 5, "legacy smoothing should report sample count");
    require(std::fabs(legacyAxes[0][0] - 0.0f) < 0.0001f, "legacy smoothing keeps first sample");
    require(std::fabs(legacyAxes[0][1] - (140.0f / 12.0f)) < 0.0001f, "legacy smoothing sample 1");
    require(std::fabs(legacyAxes[0][2] - (260.0f / 12.0f)) < 0.0001f, "legacy smoothing Blender p2 quirk");
    require(std::fabs(legacyAxes[0][3] - (340.0f / 12.0f)) < 0.0001f, "legacy smoothing sample 3");
    require(std::fabs(legacyAxes[0][4] - 40.0f) < 0.0001f, "legacy smoothing keeps last sample");

    constexpr std::uint32_t physicalRuntime = 11;
    constexpr std::uint32_t gappedRuntime = 12;
    constexpr std::uint32_t shortRuntime = 13;
    constexpr std::uint32_t spikeRuntime = 14;
    const std::uint32_t skeletalRuntime =
        ovtr::skeletalBoneRuntimeIndex(ovtr::SkeletalHandSide::Left, 6);

    std::vector<ovtr::FrameSample> frames;
    for (std::uint64_t index = 0; index < 12; ++index) {
        ovtr::FrameSample frame;
        frame.frameIndex = index;
        frame.timeSeconds = static_cast<double>(index) / 90.0;
        frame.timestampNs = index * 11'111'111ull;
        const float sign = (index % 2) == 0 ? -1.0f : 1.0f;
        frame.poses.push_back(physicalPose(physicalRuntime, 1.0f + sign * 0.08f));
        frame.poses.push_back(physicalPose(spikeRuntime, index == 6 ? 1.0f : 0.0f));
        frame.poses.push_back(physicalPose(skeletalRuntime, 5.0f + sign));

        if (index < 3) {
            frame.poses.push_back(physicalPose(shortRuntime, 2.0f + sign * 0.25f));
        }
        if (index == 4) {
            frame.poses.push_back(physicalPose(gappedRuntime, std::numeric_limits<float>::quiet_NaN(), false));
        } else {
            frame.poses.push_back(physicalPose(gappedRuntime, 3.0f + sign * 0.06f));
        }
        frames.push_back(frame);
    }

    const std::vector<ovtr::FrameSample> before = frames;
    std::vector<ovtr::FrameSample> noneRepairFrames = frames;
    const float beforeJitter = jitterSum(frames, physicalRuntime, 1.0f);
    const ovtr::win32::ExportNoiseFilterResult result = ovtr::win32::applyExportNoiseFilterToFrames(
        frames,
        90.0,
        {true, 8.0f, ovtr::win32::OutlierRepairStrength::Light, 2}
    );
    const ovtr::win32::ExportNoiseFilterResult noneRepairResult = ovtr::win32::applyExportNoiseFilterToFrames(
        noneRepairFrames,
        90.0,
        {true, 8.0f, ovtr::win32::OutlierRepairStrength::None, 0}
    );
    const ovtr::win32::ExportNoiseFilterReport& report = result.report;
    const ovtr::win32::ExportNoiseFilterReport& noneRepairReport = noneRepairResult.report;

    require(report.filteredSamples > 0, "noise filter should modify valid physical samples");
    require(!result.runDiagnostics.empty(), "noise filter should collect run diagnostics");
    require(report.outlierCandidates > 0, "outlier repair should report candidate samples");
    require(report.repairedOutliers > 0, "light outlier repair should repair a one-frame spike");
    require(report.smoothedSamples > 0, "legacy smoothing should report smoothed samples");
    require(noneRepairReport.repairedOutliers == 0, "none outlier repair should not repair spikes");
    require(report.maxObservedStepMeters > report.outlierStepThresholdMeters, "outlier report should expose step diagnostics");
    require(jitterSum(frames, physicalRuntime, 1.0f) < beforeJitter, "noise filter should reduce jitter");
    require(
        std::fabs(frames[6].poses[1].position[0]) < std::fabs(noneRepairFrames[6].poses[1].position[0]),
        "outlier repair should reduce the spike before Butterworth"
    );
    require(frames[4].poses.back().position[0] != frames[4].poses.back().position[0], "invalid NaN stays original");
    for (std::size_t index = 5; index < frames.size(); ++index) {
        require(std::isfinite(frames[index].poses.back().position[0]), "valid run after NaN stays finite");
    }
    for (std::size_t index = 0; index < 3; ++index) {
        require(
            frames[index].poses[3].position[0] == before[index].poses[3].position[0],
            "short valid run stays original"
        );
    }
    for (std::size_t index = 0; index < frames.size(); ++index) {
        require(
            frames[index].poses[2].position[0] == before[index].poses[2].position[0],
            "skeletal pose is not filtered"
        );
    }
}

} // namespace ovtr::test
#endif
