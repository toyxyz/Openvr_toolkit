#include "platform/win32/SessionSkeletonExportClip.h"

#include "platform/win32/AppConfig.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/ExportNoiseFilter.h"
#include "platform/win32/MappingCalibrationSolve.h"
#include "platform/win32/SkeletonRecording.h"
#include "recording/BinarySessionReader.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace ovtr::win32 {
namespace {

double frameTimeSeconds(
    const ovtr::FrameSample& frame,
    const std::uint64_t frameIndex,
    const double sampleRate,
    const double previousTime
) noexcept {
    if (frameIndex == 0 || frame.timeSeconds > previousTime) {
        return frame.timeSeconds;
    }
    const double rate = sampleRate > 0.0 ? sampleRate : kDefaultRecordExportSampleRate;
    return static_cast<double>(frameIndex) / rate;
}

} // namespace

bool buildSessionSkeletonClipFromRequest(
    const SessionSkeletonClipRequest& request,
    SkeletonRecordingClip& clip,
    std::string& error,
    std::vector<std::string>* warnings
) {
    error.clear();
    if (warnings) {
        warnings->clear();
    }
    BinarySessionReader reader;
    if (!reader.open(request.session.framesPath, request.session.frameIndexPath)) {
        error = reader.lastError();
        return false;
    }
    if (reader.frameCount() == 0) {
        error = "session has no frames";
        return false;
    }

    std::vector<ovtr::FrameSample> frames;
    frames.reserve(static_cast<std::size_t>(reader.frameCount()));
    for (std::uint64_t index = 0; index < reader.frameCount(); ++index) {
        ovtr::FrameSample frame;
        if (!reader.readFrame(index, frame)) {
            error = reader.lastError();
            return false;
        }
        frames.push_back(std::move(frame));
    }
    const ExportNoiseFilterSettings filterSettings{
        request.applyNoiseFilterOnExport,
        request.noiseFilterCutoffHz,
        request.outlierRepairStrength,
        request.smoothingIterations
    };
    const ExportNoiseFilterResult filterResult = applyExportNoiseFilterToFrames(
        frames,
        request.session.targetSampleRate,
        filterSettings
    );
    const ExportNoiseFilterReport& filterReport = filterResult.report;
    if (warnings && request.applyNoiseFilterOnExport) {
        warnings->push_back(
            "Noise filter applied: Butterworth " + std::to_string(request.noiseFilterCutoffHz) +
            " Hz, samples " + std::to_string(filterReport.filteredSamples) +
            ", outlier repair " + outlierRepairStrengthConfigValue(request.outlierRepairStrength) +
            ", smoothing iterations " + std::to_string(request.smoothingIterations) +
            ", candidates " + std::to_string(filterReport.outlierCandidates) +
            ", repaired " + std::to_string(filterReport.repairedOutliers) +
            ", smoothed " + std::to_string(filterReport.smoothedSamples) +
            ", skipped runs " + std::to_string(filterReport.skippedOutlierRuns)
        );
        if (filterReport.invalidOutputs > 0) {
            warnings->push_back("Noise filter warning: invalid outputs reverted to original positions");
        }
        if (filterReport.invalidRepairOutputs > 0) {
            warnings->push_back("Noise filter warning: invalid outlier repairs skipped");
        }
    }

    MappingActor actor = request.actor;
    clip = SkeletonRecordingClip{};
    clip.actorId = actor.id;
    clip.profile = actor.profile;
    clip.actorName = actor.profile.name;

    double previousTime = -1.0;
    for (std::size_t index = 0; index < frames.size(); ++index) {
        ovtr::FrameSample& frame = frames[index];
        ovtr::PosePollResult poses;
        poses.timestampNs = frame.timestampNs;
        poses.poses = std::move(frame.poses);
        if (!updateCalibratedMappingActorJoints(
                actor,
                poses,
                request.originEnabled,
                request.originOffset,
                request.originRotationDegrees
            )) {
            error = "failed to solve skeleton for session frame " + std::to_string(index);
            return false;
        }

        SkeletonRecordingFrame skeletonFrame;
        skeletonFrame.timeSeconds =
            frameTimeSeconds(frame, index, request.session.targetSampleRate, previousTime);
        skeletonFrame.pose = actor.liveSkeletonPose;
        skeletonFrame.pose.timeSeconds = skeletonFrame.timeSeconds;
        previousTime = skeletonFrame.timeSeconds;
        clip.frames.push_back(std::move(skeletonFrame));
    }
    if (!hasSkeletonRecordingFrames(clip)) {
        error = "session produced no skeleton frames";
        return false;
    }
    return true;
}

bool buildLoadedSessionSkeletonClipFromRequest(
    const LoadedSessionSkeletonClipRequest& request,
    SkeletonRecordingClip& clip,
    std::string& error
) {
    return buildSessionSkeletonClipFromRequest(request, clip, error);
}

} // namespace ovtr::win32
