#pragma once

#include "data/SessionTypes.h"

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
    bool applyEulerUnroll = true;
    FbxCoordinatePolicy coordinatePolicy = FbxCoordinatePolicy::Blender;
    FbxRotationOrder rotationOrder = FbxRotationOrder::XYZ;
};

struct ExportResult {
    bool success = false;
    std::filesystem::path outputPath;
    std::string error;
};

std::string makeFbxSafeName(const DeviceDescriptor& device);
std::array<double, 3> quaternionToEulerXyzDegrees(const std::array<float, 4>& quaternion);

ExportResult exportSessionToFbxAscii(
    const RecordingSession& session,
    const FbxExportOptions& options
);

} // namespace ovtr
