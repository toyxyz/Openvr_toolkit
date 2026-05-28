#include "platform/win32/DebugMonitorStatusLines.h"

#include "platform/win32/AppRecordingState.h"
#include "platform/win32/DebugPanel.h"
#include "platform/win32/StatusPanel.h"

#include <iomanip>
#include <mutex>
#include <sstream>

namespace ovtr::win32 {

void appendDebugMonitorRecordingLines(
    std::vector<std::wstring>& lines,
    const AppRecordingState& state,
    const std::chrono::steady_clock::time_point now
)
{
    ovtr::RecorderState recorderState = ovtr::RecorderState::Idle;
    std::uint64_t frameCount = 0;
    std::uint64_t droppedFrames = 0;
    bool recordingDelayActive = false;
    int remainingDelaySeconds = 0;
    {
        std::lock_guard<std::mutex> lock(state.recordingMutex);
        recorderState = state.recorder.state();
        frameCount = state.recorder.frameCount();
        droppedFrames = state.recordingDroppedFrames;
        recordingDelayActive = state.recordingDelayActive;
        remainingDelaySeconds = remainingRecordDelaySeconds(state, now);
    }

    std::wostringstream stream;
    stream << L"Recording: " << recorderStateText(recorderState)
           << L"   Frames: " << frameCount
           << L"   Dropped: " << droppedFrames
           << L"   Save: " << exportFormatDisplayText(state.recordSaveFormat)
           << L"   Resample: " << std::fixed << std::setprecision(3)
           << state.recordExportSampleRate << L"fps"
           << L"   Delay: " << std::fixed << std::setprecision(3)
           << state.recordDelaySeconds << L"s";
    if (recordingDelayActive) {
        stream << L"   Remaining: " << remainingDelaySeconds << L"s";
    }
    lines.emplace_back(stream.str());
}

} // namespace ovtr::win32
