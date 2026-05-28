#pragma once

#include "data/SessionTypes.h"
#include "export/ExportPoseTrack.h"
#include "export/ExportTypes.h"

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace ovtr {

enum class FbxCoordinatePolicy {
    Blender,
    OpenVRNative,
};

enum class FbxRotationOrder {
    XYZ,
};

struct FbxExportOptions {
    std::filesystem::path outputPath;
    bool includeGeometry = true;
    bool includeTrackingReference = true;
    double exportSampleRate = 0.0;
    FbxCoordinatePolicy coordinatePolicy = FbxCoordinatePolicy::Blender;
    FbxRotationOrder rotationOrder = FbxRotationOrder::XYZ;
    ExportGeometryProvider geometryProvider;
    std::vector<ExportStaticPoseTrack> staticTracks;
};

std::string makeFbxSafeName(const DeviceDescriptor& device);
std::array<double, 3> quaternionToEulerXyzDegrees(const std::array<float, 4>& quaternion);

ExportResult exportSessionToFbxAscii(
    const RecordingSession& session,
    const FbxExportOptions& options
);

} // namespace ovtr
