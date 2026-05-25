#pragma once

#include "data/SessionTypes.h"
#include "export/ExportTypes.h"

#include <array>
#include <filesystem>
#include <string>

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
};

std::string makeFbxSafeName(const DeviceDescriptor& device);
std::array<double, 3> quaternionToEulerXyzDegrees(const std::array<float, 4>& quaternion);

ExportResult exportSessionToFbxAscii(
    const RecordingSession& session,
    const FbxExportOptions& options
);

} // namespace ovtr
