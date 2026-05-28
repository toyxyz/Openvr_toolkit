#include "platform/win32/RecordingStateQueries.h"

#include "platform/win32/AppRecordingState.h"

#include <mutex>

namespace ovtr::win32 {

bool isRecorderBusyForExport(
    const bool recordingDelayActive,
    const RecorderState recorderState
) noexcept
{
    return recordingDelayActive ||
        recorderState == RecorderState::Recording ||
        recorderState == RecorderState::Paused ||
        recorderState == RecorderState::Starting ||
        recorderState == RecorderState::Stopping ||
        recorderState == RecorderState::Finalizing;
}

bool isRecorderBusyForExport(const AppRecordingState& state)
{
    std::lock_guard<std::mutex> lock(state.recordingMutex);
    return isRecorderBusyForExport(state.recordingDelayActive, state.recorder.state());
}

std::string exportBlockReason(
    const bool recordingDelayActive,
    const RecorderState recorderState,
    const std::filesystem::path& currentSessionFolder
)
{
    if (isRecorderBusyForExport(recordingDelayActive, recorderState)) {
        return "stop recording before exporting";
    }
    if (currentSessionFolder.empty()) {
        return "no recorded session available";
    }
    return {};
}

std::string exportBlockReason(const AppRecordingState& state)
{
    std::lock_guard<std::mutex> lock(state.recordingMutex);
    return exportBlockReason(
        state.recordingDelayActive,
        state.recorder.state(),
        state.currentSessionFolder
    );
}

bool isRecordingControlActive(
    const bool recordingDelayActive,
    const RecorderState recorderState
) noexcept
{
    return recordingDelayActive ||
        recorderState == RecorderState::Recording ||
        recorderState == RecorderState::Paused;
}

bool isRecordingControlActive(const AppRecordingState& state)
{
    std::lock_guard<std::mutex> lock(state.recordingMutex);
    return isRecordingControlActive(state.recordingDelayActive, state.recorder.state());
}

} // namespace ovtr::win32
