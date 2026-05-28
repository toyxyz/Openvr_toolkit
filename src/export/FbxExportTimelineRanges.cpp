#include "export/FbxExportTimelineRanges.h"

namespace ovtr::detail {

std::vector<FbxTimelineKeyRange> collectFbxTimelineKeyRanges(const std::vector<FbxDeviceExport>& devices)
{
    std::vector<FbxTimelineKeyRange> ranges;
    ranges.reserve(devices.size());
    for (const FbxDeviceExport& device : devices) {
        if (device.keys.empty()) {
            continue;
        }

        FbxTimelineKeyRange range;
        range.startSeconds = device.keys.front().timeSeconds;
        range.stopSeconds = device.keys.back().timeSeconds;
        if (device.keys.size() >= 2) {
            range.firstIntervalSeconds = device.keys[1].timeSeconds - device.keys[0].timeSeconds;
            range.hasFrameInterval = true;
        }
        ranges.push_back(range);
    }
    return ranges;
}

} // namespace ovtr::detail
