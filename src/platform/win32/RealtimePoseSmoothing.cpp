#include "platform/win32/RealtimePoseSmoothing.h"

#include "data/SkeletalSyntheticPose.h"
#include "data/VmcSyntheticPose.h"
#include "platform/win32/ConfigStore.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace ovtr::win32 {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr double kMaxContinuityGapSeconds = 0.25;

bool finitePosition(const ovtr::PoseSample& pose) noexcept
{
    return std::isfinite(pose.position[0]) &&
        std::isfinite(pose.position[1]) &&
        std::isfinite(pose.position[2]);
}

bool filterablePose(const ovtr::PoseSample& pose) noexcept
{
    return !ovtr::isSkeletalBoneRuntimeIndex(pose.runtimeIndex) &&
        !ovtr::isVmcFingerRuntimeIndex(pose.runtimeIndex) &&
        (pose.flags & ovtr::PoseFlagPoseValid) != 0 &&
        finitePosition(pose);
}

float smoothingAlpha(const float cutoffHz, const double dtSeconds) noexcept
{
    const float safeCutoff = std::max(0.001f, cutoffHz);
    const double tau = 1.0 / (2.0 * static_cast<double>(kPi) * safeCutoff);
    return static_cast<float>(1.0 / (1.0 + tau / std::max(dtSeconds, 1.0e-6)));
}

float lerp(const float previous, const float current, const float alpha) noexcept
{
    return previous + (current - previous) * alpha;
}

} // namespace

OneEuroPresetParameters oneEuroParametersForPreset(const RealtimeSmoothingPreset preset) noexcept
{
    switch (preset) {
    case RealtimeSmoothingPreset::VeryLight:
        return {2.5f, 0.03f, 1.0f};
    case RealtimeSmoothingPreset::Light:
        return {1.5f, 0.04f, 1.0f};
    case RealtimeSmoothingPreset::Strong:
        return {0.7f, 0.07f, 1.0f};
    case RealtimeSmoothingPreset::VeryStrong:
        return {0.45f, 0.09f, 1.0f};
    case RealtimeSmoothingPreset::Normal:
    default:
        return {1.0f, 0.05f, 1.0f};
    }
}

const char* realtimeSmoothingPresetLabel(const RealtimeSmoothingPreset preset) noexcept
{
    switch (preset) {
    case RealtimeSmoothingPreset::VeryLight:
        return "Very Light";
    case RealtimeSmoothingPreset::Light:
        return "Light";
    case RealtimeSmoothingPreset::Strong:
        return "Strong";
    case RealtimeSmoothingPreset::VeryStrong:
        return "Very Strong";
    case RealtimeSmoothingPreset::Normal:
    default:
        return "Normal";
    }
}

const char* realtimeSmoothingPresetConfigValue(const RealtimeSmoothingPreset preset) noexcept
{
    switch (preset) {
    case RealtimeSmoothingPreset::VeryLight:
        return "very_light";
    case RealtimeSmoothingPreset::Light:
        return "light";
    case RealtimeSmoothingPreset::Strong:
        return "strong";
    case RealtimeSmoothingPreset::VeryStrong:
        return "very_strong";
    case RealtimeSmoothingPreset::Normal:
    default:
        return "normal";
    }
}

bool parseRealtimeSmoothingPresetConfigValue(const std::string& value, RealtimeSmoothingPreset& out)
{
    const std::string lowered = lowerAscii(trimAscii(value));
    if (lowered == "very_light" || lowered == "very light" || lowered == "verylight") {
        out = RealtimeSmoothingPreset::VeryLight;
        return true;
    }
    if (lowered == "light") {
        out = RealtimeSmoothingPreset::Light;
        return true;
    }
    if (lowered == "strong") {
        out = RealtimeSmoothingPreset::Strong;
        return true;
    }
    if (lowered == "very_strong" || lowered == "very strong" || lowered == "verystrong") {
        out = RealtimeSmoothingPreset::VeryStrong;
        return true;
    }
    if (lowered == "normal") {
        out = RealtimeSmoothingPreset::Normal;
        return true;
    }
    return false;
}

void RealtimePoseSmoother::reset()
{
    filters_.clear();
}

void RealtimePoseSmoother::setPreset(const RealtimeSmoothingPreset preset)
{
    if (preset_ != preset) {
        preset_ = preset;
        reset();
    }
}

void RealtimePoseSmoother::apply(ovtr::PosePollResult& poses)
{
    const OneEuroPresetParameters params = oneEuroParametersForPreset(preset_);
    for (ovtr::PoseSample& pose : poses.poses) {
        if (!filterablePose(pose)) {
            filters_.erase(pose.runtimeIndex);
            continue;
        }

        PoseFilterState& state = filters_[pose.runtimeIndex];
        const double dt = state.lastTimestampNs > 0 && poses.timestampNs > state.lastTimestampNs
            ? static_cast<double>(poses.timestampNs - state.lastTimestampNs) / 1'000'000'000.0
            : 0.0;
        if (!state.initialized || dt <= 0.0 || dt > kMaxContinuityGapSeconds) {
            state.lastRaw = pose.position;
            state.filtered = pose.position;
            state.filteredDerivative = {0.0f, 0.0f, 0.0f};
            state.lastTimestampNs = poses.timestampNs;
            state.initialized = true;
            continue;
        }

        const float derivativeAlpha = smoothingAlpha(params.derivativeCutoffHz, dt);
        float derivativeMagnitude = 0.0f;
        for (int axis = 0; axis < 3; ++axis) {
            const float derivative = static_cast<float>((pose.position[axis] - state.lastRaw[axis]) / dt);
            state.filteredDerivative[axis] = lerp(state.filteredDerivative[axis], derivative, derivativeAlpha);
            derivativeMagnitude += state.filteredDerivative[axis] * state.filteredDerivative[axis];
        }
        derivativeMagnitude = std::sqrt(derivativeMagnitude);

        const float cutoff = params.minCutoffHz + params.beta * derivativeMagnitude;
        const float positionAlpha = smoothingAlpha(cutoff, dt);
        for (int axis = 0; axis < 3; ++axis) {
            state.filtered[axis] = lerp(state.filtered[axis], pose.position[axis], positionAlpha);
        }
        state.lastRaw = pose.position;
        state.lastTimestampNs = poses.timestampNs;
        pose.position = state.filtered;
    }
}

} // namespace ovtr::win32
