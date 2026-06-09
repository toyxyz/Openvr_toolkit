#pragma once

#include "data/SessionTypes.h"
#include "platform/win32/AppProfileState.h"
#include "platform/win32/ConfigTypes.h"
#include "platform/win32/SkeletonRecordingTypes.h"

#include <array>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct SessionSkeletonClipRequest {
    ovtr::RecordingSession session;
    MappingActor actor;
    bool originEnabled = false;
    std::array<float, 3> originOffset{};
    std::array<float, 3> originRotationDegrees{};
    bool applyNoiseFilterOnExport = false;
    float noiseFilterCutoffHz = 8.0f;
    OutlierRepairStrength outlierRepairStrength = OutlierRepairStrength::Light;
    int smoothingIterations = kDefaultSmoothingIterations;
};

bool buildSessionSkeletonClipFromRequest(
    const SessionSkeletonClipRequest& request,
    SkeletonRecordingClip& clip,
    std::string& error,
    std::vector<std::string>* warnings = nullptr
);

using LoadedSessionSkeletonClipRequest = SessionSkeletonClipRequest;

bool buildLoadedSessionSkeletonClipFromRequest(
    const LoadedSessionSkeletonClipRequest& request,
    SkeletonRecordingClip& clip,
    std::string& error
);

} // namespace ovtr::win32
