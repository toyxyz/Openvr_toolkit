#pragma once

#include <cstdint>
#include <vector>

namespace ovtr {

struct FbxTimelineSettings {
    double frameRate = 0.0;
    int timeMode = 0;
    double customFrameRate = -1.0;
    std::int64_t startTime = 0;
    std::int64_t stopTime = 0;
};

struct FbxTimelineKeyRange {
    double startSeconds = 0.0;
    double stopSeconds = 0.0;
    double firstIntervalSeconds = 0.0;
    bool hasFrameInterval = false;
};

FbxTimelineSettings makeFbxTimelineSettings(
    const std::vector<FbxTimelineKeyRange>& keyRanges,
    double requestedFrameRate,
    double sessionFrameRate
);

} // namespace ovtr
