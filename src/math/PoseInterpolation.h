#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace ovtr {

inline constexpr std::size_t kMaxResampledKeyframeCount = 1000000;

double clamp01(double value);
std::array<float, 3> lerpVec3(const std::array<float, 3>& from, const std::array<float, 3>& to, double factor);
std::array<double, 3> lerpVec3(const std::array<double, 3>& from, const std::array<double, 3>& to, double factor);
double quaternionDot(const std::array<float, 4>& left, const std::array<float, 4>& right);
std::array<float, 4> slerpQuaternion(
    const std::array<float, 4>& from,
    const std::array<float, 4>& to,
    double factor
);

template <typename Key, typename Interpolate>
void resampleKeyframesByTime(std::vector<Key>& keys, const double sampleRate, Interpolate interpolate)
{
    if (sampleRate <= 0.0 || keys.size() < 2) {
        return;
    }

    const double interval = 1.0 / sampleRate;
    const double startTime = keys.front().timeSeconds;
    const double endTime = keys.back().timeSeconds;
    if (!std::isfinite(interval) || interval <= 0.0 || endTime <= startTime) {
        return;
    }

    std::vector<Key> source = std::move(keys);
    std::vector<Key> resampled;
    const std::size_t estimatedCount =
        static_cast<std::size_t>(std::floor((endTime - startTime) / interval)) + 2;
    if (estimatedCount > kMaxResampledKeyframeCount) {
        keys = std::move(source);
        return;
    }
    resampled.reserve(estimatedCount);

    std::size_t upperIndex = 1;
    constexpr double epsilon = 1.0e-9;
    for (double time = startTime; time <= endTime + epsilon; time += interval) {
        const double sampleTime = std::min(time, endTime);
        while (upperIndex + 1 < source.size() && source[upperIndex].timeSeconds < sampleTime) {
            ++upperIndex;
        }
        resampled.push_back(interpolate(source[upperIndex - 1], source[upperIndex], sampleTime));
    }

    if (!resampled.empty() && endTime - resampled.back().timeSeconds > interval * 0.25) {
        resampled.push_back(source.back());
    }

    keys = std::move(resampled);
}

} // namespace ovtr
