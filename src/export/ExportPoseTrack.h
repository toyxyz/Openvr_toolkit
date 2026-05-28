#pragma once

#include "data/SessionTypes.h"
#include "export/RenderModelGeometry.h"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace ovtr {

struct ExportPoseKey {
    double timeSeconds = 0.0;
    std::array<float, 3> translation{0.0f, 0.0f, 0.0f};
    std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};
};

struct ExportPoseTrack {
    DeviceDescriptor device;
    std::string nodeName;
    bool hasParentRuntimeIndex = false;
    std::uint32_t parentRuntimeIndex = 0;
    std::vector<ExportPoseKey> keys;
    RenderModelGeometry geometry;
};

struct ExportStaticPoseTrack {
    DeviceDescriptor device;
    std::string nodeName;
    std::array<float, 3> translation{0.0f, 0.0f, 0.0f};
    std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};
    RenderModelGeometry geometry;
};

using ExportGeometryProvider = std::function<RenderModelGeometry(const DeviceDescriptor&)>;

struct ExportPoseTrackOptions {
    bool includeGeometry = true;
    bool includeTrackingReference = true;
    ExportGeometryProvider geometryProvider;
    std::vector<ExportStaticPoseTrack> staticTracks;
};

bool collectExportPoseTracks(
    const RecordingSession& session,
    const ExportPoseTrackOptions& options,
    std::vector<ExportPoseTrack>& tracks,
    std::string& error
);

} // namespace ovtr
