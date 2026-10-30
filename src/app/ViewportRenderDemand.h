#pragma once

#include <cstddef>
#include <cstdint>

namespace ovtr {

// Display invalidation is independent of pose capture and application updates.
struct ViewportRenderDemand {
    bool dirty = true;
    std::uint64_t poseTimestamp = 0;
    std::size_t poseCount = 0;
    double sceneSeconds = -1.0;
    bool overlayWasAnimated = false;

    bool needsRender(const bool visible, const std::uint64_t timestamp,
        const std::size_t count, const double seconds, const bool animatedOverlay) const noexcept
    {
        return visible && (dirty || animatedOverlay || animatedOverlay != overlayWasAnimated || count != poseCount ||
            (count != 0 && timestamp != poseTimestamp) || seconds != sceneSeconds);
    }
    void presented(const std::uint64_t timestamp, const std::size_t count, const double seconds,
        const bool animatedOverlay = false) noexcept
    {
        dirty = false;
        poseTimestamp = timestamp;
        poseCount = count;
        sceneSeconds = seconds;
        overlayWasAnimated = animatedOverlay;
    }
};

} // namespace ovtr
