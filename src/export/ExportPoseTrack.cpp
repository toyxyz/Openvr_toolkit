#include "export/ExportPoseTrack.h"

#include "data/SkeletalSyntheticPose.h"
#include "math/QuaternionUtils.h"
#include "recording/BinarySessionReader.h"
#include "util/Identifier.h"

#include <unordered_map>
#include <utility>

namespace ovtr {
namespace {

void appendStaticPoseTracks(
    const std::vector<ExportStaticPoseTrack>& staticTracks,
    const bool includeGeometry,
    const bool hasRecordedFrame,
    const double firstTimeSeconds,
    const double lastTimeSeconds,
    std::vector<ExportPoseTrack>& tracks
)
{
    for (const ExportStaticPoseTrack& source : staticTracks) {
        ExportPoseTrack track;
        track.device = source.device;
        track.nodeName = source.nodeName.empty() ? makeDeviceSafeName(source.device) : source.nodeName;
        if (includeGeometry) {
            track.geometry = source.geometry;
        }
        const ExportPoseKey key{hasRecordedFrame ? firstTimeSeconds : 0.0, source.translation, normalizeQuaternion(source.rotation)};
        track.keys.push_back(key);
        if (hasRecordedFrame && lastTimeSeconds > firstTimeSeconds) {
            track.keys.push_back({lastTimeSeconds, source.translation, normalizeQuaternion(source.rotation)});
        }
        tracks.push_back(std::move(track));
    }
}

RenderModelGeometry geometryForDevice(
    const DeviceDescriptor& descriptor,
    const ExportPoseTrackOptions& options
)
{
    if (descriptor.renderModelName == kSkeletalBoneBoxRenderModelName) {
        return makeBoxRenderModelGeometry(kSkeletalBoneBoxEdgeMeters);
    }
    if (options.geometryProvider) {
        return options.geometryProvider(descriptor);
    }
    return loadSteamVRRenderModelGeometry(descriptor.renderModelName);
}

ExportPoseTrack makeRecordedPoseTrack(
    const DeviceDescriptor& descriptor,
    const ExportPoseTrackOptions& options
)
{
    ExportPoseTrack track;
    track.device = descriptor;
    track.nodeName = makeDeviceSafeName(descriptor);
    if (options.includeGeometry) {
        track.geometry = geometryForDevice(descriptor, options);
        track.geometry.available = track.geometry.available ||
            (!track.geometry.vertices.empty() && !track.geometry.indices.empty());
    }
    return track;
}

bool makeSkeletalPoseTrackForRuntimeIndex(
    const std::uint32_t runtimeIndex,
    const ExportPoseTrackOptions& options,
    ExportPoseTrack& outTrack
)
{
    SkeletalHandSide side = SkeletalHandSide::Left;
    std::uint32_t boneIndex = 0;
    if (!decodeSkeletalBoneRuntimeIndex(runtimeIndex, side, boneIndex)) {
        return false;
    }
    if (!shouldRecordSkeletalBoneIndex(boneIndex)) {
        return false;
    }
    outTrack = makeRecordedPoseTrack(makeSkeletalBoneDeviceDescriptor(side, boneIndex), options);
    outTrack.nodeName = skeletalBoneNodeName(side, boneIndex);
    return true;
}

} // namespace

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

    tracks.reserve(session.devices.size() + options.staticTracks.size());
    for (const DeviceDescriptor& descriptor : session.devices) {
        if (!options.includeTrackingReference && descriptor.deviceClass == DeviceClass::TrackingReference) {
            continue;
        }

        tracks.push_back(makeRecordedPoseTrack(descriptor, options));
    }

    std::unordered_map<std::uint32_t, std::size_t> runtimeIndexToTrack;
    for (std::size_t index = 0; index < tracks.size(); ++index) {
        runtimeIndexToTrack[tracks[index].device.runtimeIndex] = index;
    }

    bool hasRecordedFrame = false;
    double firstTimeSeconds = 0.0;
    double lastTimeSeconds = 0.0;
    for (std::uint64_t frameIndex = 0; frameIndex < reader.frameCount(); ++frameIndex) {
        FrameSample frame;
        if (!reader.readFrame(frameIndex, frame)) {
            error = "failed to read recorded frame: " + reader.lastError();
            return false;
        }
        if (!hasRecordedFrame) {
            firstTimeSeconds = frame.timeSeconds;
            hasRecordedFrame = true;
        }
        lastTimeSeconds = frame.timeSeconds;

        for (const PoseSample& pose : frame.poses) {
            if ((pose.flags & PoseFlagPoseValid) == 0) {
                continue;
            }

            auto found = runtimeIndexToTrack.find(pose.runtimeIndex);
            if (found == runtimeIndexToTrack.end()) {
                ExportPoseTrack skeletalTrack;
                if (!makeSkeletalPoseTrackForRuntimeIndex(pose.runtimeIndex, options, skeletalTrack)) {
                    continue;
                }
                const std::size_t trackIndex = tracks.size();
                runtimeIndexToTrack[pose.runtimeIndex] = trackIndex;
                tracks.push_back(std::move(skeletalTrack));
                found = runtimeIndexToTrack.find(pose.runtimeIndex);
            }

            tracks[found->second].keys.push_back({
                frame.timeSeconds,
                pose.position,
                normalizeQuaternion(pose.rotation),
            });
        }
    }

    appendStaticPoseTracks(
        options.staticTracks,
        options.includeGeometry,
        hasRecordedFrame,
        firstTimeSeconds,
        lastTimeSeconds,
        tracks
    );
    return true;
}

} // namespace ovtr
