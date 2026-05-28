#include "export/GltfExporter.h"

#include "export/GltfExportSceneBuilder.h"
#include "export/GltfFileWriter.h"
#include "export/GltfJsonBuilder.h"
#include "export/SkeletalExportHierarchy.h"
#include "util/BinaryBuffer.h"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace ovtr {

ExportResult exportSessionToGltf(const RecordingSession& session, const GltfExportOptions& options)
{
    ExportResult result;
    result.outputPath = options.outputPath;

    if (options.outputPath.empty()) {
        result.error = "glTF output path is empty";
        return result;
    }

    if (!isValidExportSampleRate(options.exportSampleRate)) {
        result.error = "glTF export sample rate must be 0 or between 0 and 1000 FPS";
        return result;
    }

    std::vector<ExportPoseTrack> tracks;
    std::string trackError;
    if (!collectExportPoseTracks(
            session,
            {
                options.includeGeometry,
                options.includeTrackingReference,
                options.geometryProvider,
                options.staticTracks
            },
            tracks,
            trackError
        )) {
        result.error = trackError;
        return result;
    }

    std::string hierarchyError;
    if (!applySkeletalExportHierarchy(tracks, hierarchyError)) {
        result.error = hierarchyError;
        return result;
    }

    GltfExportScene scene;
    buildGltfExportScene(std::move(tracks), options.exportSampleRate, scene);

    std::string directoryError;
    if (!ensureParentDirectory(options.outputPath, &directoryError)) {
        result.error = "failed to create glTF export directory: " + directoryError;
        return result;
    }

    if (options.format == GltfExportFormat::Glb) {
        const std::string json = makeGltfJson(
            session,
            scene.devices,
            scene.meshes,
            scene.bufferViews,
            scene.accessors,
            scene.animationTargets,
            scene.binary.size(),
            ""
        );
        if (!writeGltfGlbFile(options.outputPath, json, scene.binary)) {
            result.error = "failed to write GLB output file";
            return result;
        }
    } else {
        const std::filesystem::path binPath = gltfSiblingBinPath(options.outputPath);
        const std::string binUri = binPath.filename().string();
        const std::string json = makeGltfJson(
            session,
            scene.devices,
            scene.meshes,
            scene.bufferViews,
            scene.accessors,
            scene.animationTargets,
            scene.binary.size(),
            binUri
        );
        if (!writeGltfBinaryFile(binPath, scene.binary)) {
            result.error = "failed to write glTF binary buffer";
            return result;
        }
        if (!writeGltfTextFile(options.outputPath, json)) {
            result.error = "failed to write glTF JSON output file";
            return result;
        }
    }

    result.success = true;
    return result;
}

} // namespace ovtr
