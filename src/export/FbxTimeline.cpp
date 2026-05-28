#include "export/FbxTimeline.h"

#include "export/FbxAsciiMath.h"

#include <algorithm>
#include <cmath>

namespace ovtr {
namespace {

bool nearlyEqual(const double left, const double right)
{
    return std::fabs(left - right) < 0.0001;
}

int fbxTimeModeForFrameRate(const double frameRate)
{
    if (nearlyEqual(frameRate, 120.0)) {
        return 1;
    }
    if (nearlyEqual(frameRate, 100.0)) {
        return 2;
    }
    if (nearlyEqual(frameRate, 60.0)) {
        return 3;
    }
    if (nearlyEqual(frameRate, 50.0)) {
        return 4;
    }
    if (nearlyEqual(frameRate, 48.0)) {
        return 5;
    }
    if (nearlyEqual(frameRate, 30.0)) {
        return 6;
    }
    if (nearlyEqual(frameRate, 25.0)) {
        return 10;
    }
    if (nearlyEqual(frameRate, 24.0)) {
        return 11;
    }
    if (nearlyEqual(frameRate, 1000.0)) {
        return 12;
    }
    if (nearlyEqual(frameRate, 96.0)) {
        return 15;
    }
    if (nearlyEqual(frameRate, 72.0)) {
        return 16;
    }
    if (nearlyEqual(frameRate, 60000.0 / 1001.0)) {
        return 17;
    }
    return 14;
}

double inferFrameRateFromRanges(const std::vector<FbxTimelineKeyRange>& keyRanges)
{
    for (const FbxTimelineKeyRange& range : keyRanges) {
        if (range.hasFrameInterval && range.firstIntervalSeconds > 0.0 &&
            std::isfinite(range.firstIntervalSeconds)) {
            return 1.0 / range.firstIntervalSeconds;
        }
    }

    return 0.0;
}

} // namespace

FbxTimelineSettings makeFbxTimelineSettings(
    const std::vector<FbxTimelineKeyRange>& keyRanges,
    const double requestedFrameRate,
    const double sessionFrameRate
)
{
    FbxTimelineSettings settings;
    if (requestedFrameRate > 0.0) {
        settings.frameRate = requestedFrameRate;
    } else if (sessionFrameRate > 0.0) {
        settings.frameRate = sessionFrameRate;
    } else {
        settings.frameRate = inferFrameRateFromRanges(keyRanges);
    }

    if (settings.frameRate > 0.0 && std::isfinite(settings.frameRate)) {
        settings.timeMode = fbxTimeModeForFrameRate(settings.frameRate);
        settings.customFrameRate = settings.timeMode == 14 ? settings.frameRate : -1.0;
    }

    double startSeconds = 0.0;
    double stopSeconds = 0.0;
    bool hasKey = false;
    for (const FbxTimelineKeyRange& range : keyRanges) {
        if (!hasKey) {
            startSeconds = range.startSeconds;
            stopSeconds = range.stopSeconds;
            hasKey = true;
        } else {
            startSeconds = std::min(startSeconds, range.startSeconds);
            stopSeconds = std::max(stopSeconds, range.stopSeconds);
        }
    }

    settings.startTime = fbxSecondsToTicks(hasKey ? startSeconds : 0.0);
    settings.stopTime = fbxSecondsToTicks(hasKey ? stopSeconds : 0.0);
    return settings;
}

} // namespace ovtr
