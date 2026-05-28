#include "export/ExportPoseTrack.h"

#include "math/QuaternionUtils.h"
#include "recording/BinarySessionReader.h"
#include "util/Identifier.h"

#include <unordered_map>
#include <utility>

namespace ovtr {

bool collectExportPoseTracks(
    const RecordingSession& session,
    const ExportPoseTrackOptions& options,
    std::vector<ExportPoseTrack>& tracks,
    std::string& error
)
{
    tracks.clear();
    error.clear();

    BinarySessionReader reader;
    if (!reader.open(session.framesPath, session.frameIndexPath)) {
        error = "failed to open recorded session: " + reader.lastError();
        return false;
    }

    tracks.reserve(session.devices.size());
    for (const DeviceDescriptor& descriptor : session.devices) {
        if (!options.includeTrackingReference && descriptor.deviceClass == DeviceClass::TrackingReference) {
            continue;
        }

        ExportPoseTrack track;
        track.device = descriptor;
        track.nodeName = makeDeviceSafeName(descriptor);
        if (options.includeGeometry) {
            if (options.geometryProvider) {
                track.geometry = options.geometryProvider(descriptor);
            } else {
                track.geometry = loadSteamVRRenderModelGeometry(descriptor.renderModelName);
            }
            track.geometry.available = track.geometry.available ||
                (!track.geometry.vertices.empty() && !track.geometry.indices.empty());
        }
        tracks.push_back(std::move(track));
    }

    std::unordered_map<std::uint32_t, std::size_t> runtimeIndexToTrack;
    for (std::size_t index = 0; index < tracks.size(); ++index) {
        runtimeIndexToTrack[tracks[index].device.runtimeIndex] = index;
    }

    for (std::uint64_t frameIndex = 0; frameIndex < reader.frameCount(); ++frameIndex) {
        FrameSample frame;
        if (!reader.readFrame(frameIndex, frame)) {
            error = "failed to read recorded frame: " + reader.lastError();
            return false;
        }

        for (const PoseSample& pose : frame.poses) {
            if ((pose.flags & PoseFlagPoseValid) == 0) {
                continue;
            }

            const auto found = runtimeIndexToTrack.find(pose.runtimeIndex);
            if (found == runtimeIndexToTrack.end()) {
                continue;
            }

            tracks[found->second].keys.push_back({
                frame.timeSeconds,
                pose.position,
                normalizeQuaternion(pose.rotation),
            });
        }
    }

    return true;
}

} // namespace ovtr
