#include "export/FbxAsciiExporter.h"

#include "export/FbxAsciiMath.h"
#include "export/FbxExportFileWriter.h"
#include "export/FbxExportSceneBuilder.h"
#include "util/Identifier.h"
#include "util/BinaryBuffer.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace ovtr {

std::string makeFbxSafeName(const DeviceDescriptor& device)
{
    return makeDeviceSafeName(device);
}

std::array<double, 3> quaternionToEulerXyzDegrees(const std::array<float, 4>& quaternion)
{
    const FbxMatrix3 matrix = fbxQuaternionToMatrix3(quaternion);
    return fbxEulerRadiansToDegrees(fbxMatrix3ToEulerXyzRadians(matrix));
}

ExportResult exportSessionToFbxAscii(const RecordingSession& session, const FbxExportOptions& options)
{
    ExportResult result;
    result.outputPath = options.outputPath;

    if (options.rotationOrder != FbxRotationOrder::XYZ) {
        result.error = "unsupported FBX export option";
        return result;
    }

    if (options.outputPath.empty()) {
        result.error = "FBX output path is empty";
        return result;
    }

    if (!isValidExportSampleRate(options.exportSampleRate)) {
        result.error = "FBX export sample rate must be 0 or between 0 and 1000 FPS";
        return result;
    }

    std::vector<ExportPoseTrack> tracks;
    std::string trackError;
    if (!collectExportPoseTracks(
            session,
            {options.includeGeometry, options.includeTrackingReference, options.geometryProvider},
            tracks,
            trackError
        )) {
        result.error = trackError;
        return result;
    }

    FbxExportScene scene;
    buildFbxExportScene(
        std::move(tracks),
        options.coordinatePolicy,
        options.exportSampleRate,
        session.targetSampleRate,
        scene
    );

    std::string directoryError;
    if (!ensureParentDirectory(options.outputPath, &directoryError)) {
        result.error = "failed to create FBX export directory: " + directoryError;
        return result;
    }

    std::ofstream out(options.outputPath, std::ios::trunc);
    if (!out.is_open()) {
        result.error = "failed to open FBX output file";
        return result;
    }

    std::string writeError;
    if (!writeFbxExportScene(out, options.coordinatePolicy, scene, writeError)) {
        result.error = writeError;
        return result;
    }

    result.success = true;
    return result;
}

} // namespace ovtr
