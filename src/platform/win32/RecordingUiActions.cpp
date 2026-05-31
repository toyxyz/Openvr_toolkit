#include "platform/win32/RecordingUiActions.h"

#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/RecordingStartActions.h"
#include "platform/win32/SkeletonRecording.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

#include <chrono>
#include <mutex>
#include <sstream>

namespace ovtr::win32 {

void updateDelayedRecordingStart(HWND hwnd, AppWindowState& state)
{
    if (!state.recordingDelayActive) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (now < state.recordingDelayDeadline) {
        return;
    }

    appendDebugLog(state, L"Record delay elapsed");
    startRecordingNow(hwnd, state);
}

void toggleRecording(HWND hwnd)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return;
    }

    bool canceledDelay = false;
    bool stopAttempted = false;
    bool stopSucceeded = false;
    ExportFormat exportFormat = state->recordSaveFormat;
    std::uint64_t stoppedFrameCount = 0;
    std::uint64_t stoppedDroppedFrames = 0;
    std::string stopError;

    {
        std::lock_guard<std::mutex> lock(state->recordingMutex);
        state->recordingError.clear();

        if (state->recordingDelayActive) {
            state->recordingDelayActive = false;
            canceledDelay = true;
        } else if (state->recorder.state() == ovtr::RecorderState::Recording ||
                   state->recorder.state() == ovtr::RecorderState::Paused) {
            stopAttempted = true;
            const auto now = std::chrono::steady_clock::now();
            const double durationSeconds = std::chrono::duration<double>(now - state->recordingStart).count();
            stopSucceeded = state->recorder.stop(durationSeconds, state->recordingDroppedFrames);
            if (stopSucceeded) {
                finishSkeletonRecording(state->skeletonRecording);
                stoppedFrameCount = state->recorder.frameCount();
                stoppedDroppedFrames = state->recordingDroppedFrames;
            } else {
                state->recordingError = state->recorder.lastError();
                stopError = state->recordingError;
            }
        } else if (state->recorder.state() != ovtr::RecorderState::Idle &&
                   state->recorder.state() != ovtr::RecorderState::Error) {
            stopError = "busy";
        }
    }

    if (canceledDelay) {
        appendDebugLog(*state, L"Recording start canceled");
        invalidateStatusPanel(hwnd);
        return;
    }

    if (stopAttempted) {
        appendDebugLog(*state, L"Stopping recording");
        if (stopSucceeded) {
            std::wostringstream stream;
            stream << L"Recording stopped: frames " << stoppedFrameCount
                   << L", dropped " << stoppedDroppedFrames;
            appendDebugLog(*state, stream.str());
            exportCurrentSession(hwnd, exportFormat);
        } else {
            appendDebugLog(*state, "Recording stop failed: " + stopError);
        }
        invalidateStatusPanel(hwnd);
        return;
    }

    if (stopError == "busy") {
        appendDebugLog(*state, L"Recording toggle ignored: recorder is busy");
        return;
    }

    state->exportStatusMessage.clear();
    const float recordDelaySeconds = sanitizedRecordDelaySeconds(state->recordDelaySeconds);
    if (recordDelaySeconds > 0.0f) {
        const auto delayDuration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double>(recordDelaySeconds)
        );
        state->recordingDelayActive = true;
        state->recordingDelayDeadline = std::chrono::steady_clock::now() + delayDuration;
        appendDebugLog(*state, L"Recording scheduled after " + formatFloatText(recordDelaySeconds) + L"s");
        invalidateStatusPanel(hwnd);
        return;
    }

    startRecordingNow(hwnd, *state);
}

} // namespace ovtr::win32
