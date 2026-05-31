#include "platform/win32/RecordingStartActions.h"

#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/ImportedSceneActions.h"
#include "platform/win32/RecordingStartPlan.h"
#include "platform/win32/SkeletonRecording.h"
#include "platform/win32/WindowLayout.h"
#include "recording/SamplingScheduler.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace ovtr::win32 {
namespace {

constexpr double kTargetViewportFps = 90.0;

std::string localTimestampForPath()
{
    const std::time_t currentTime = std::time(nullptr);
    std::tm localTime{};
    localtime_s(&localTime, &currentTime);

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y_%m_%d_%H%M%S");
    return stream.str();
}

std::string utcTimestampIso()
{
    const std::time_t currentTime = std::time(nullptr);
    std::tm utcTime{};
    gmtime_s(&utcTime, &currentTime);

    std::ostringstream stream;
    stream << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

const MappingActor* skeletonRecordingActor(const AppWindowState& state) noexcept
{
    for (const MappingActor& actor : state.mappingActors) {
        if (actor.calibrated && actor.id == state.selectedMappingActorId) {
            return &actor;
        }
    }
    for (const MappingActor& actor : state.mappingActors) {
        if (actor.calibrated) {
            return &actor;
        }
    }
    return nullptr;
}

} // namespace

void startRecordingNow(HWND hwnd, AppWindowState& state)
{
    const RecordingStartPlan plan = makeRecordingStartPlan(
        static_cast<const AppRuntimeState&>(state),
        static_cast<const AppDeviceState&>(state),
        std::filesystem::current_path() / "recordings",
        "session_" + localTimestampForPath(),
        utcTimestampIso(),
        kTargetViewportFps
    );

    bool recordingStarted = false;
    std::string recordingError;
    {
        std::lock_guard<std::mutex> lock(state.recordingMutex);
        state.recordingDelayActive = false;
        state.exportStatusMessage.clear();
        state.currentSessionFolder = plan.sessionFolder;
        state.recordingStart = std::chrono::steady_clock::now();
        state.recordingDroppedFrames = 0;
        state.recordingScheduler = ovtr::SamplingScheduler(kTargetViewportFps);
        state.recordingScheduler.reset(state.recordingStart);
        state.skeletonRecording = SkeletonRecordingClip{};

        recordingStarted = state.recorder.start(plan.options);
        if (!recordingStarted) {
            state.recordingError = state.recorder.lastError();
            recordingError = state.recordingError;
        } else if (const MappingActor* actor = skeletonRecordingActor(state)) {
            beginSkeletonRecording(state.skeletonRecording, *actor);
        }
    }

    appendDebugLog(state, "Starting recording: " + plan.sessionFolder.string());
    if (recordingStarted) {
        appendDebugLog(state, L"Recording started");
        if (state.skeletonRecording.active) {
            appendDebugLog(state, L"Skeleton BVH capture started");
        } else {
            appendDebugLog(state, L"Skeleton BVH capture skipped: no calibrated Mapping actor");
        }
        startImportedGlbPlaybackForRecording(hwnd, state);
    } else {
        appendDebugLog(state, "Recording start failed: " + recordingError);
    }

    invalidateStatusPanel(hwnd);
}

} // namespace ovtr::win32
