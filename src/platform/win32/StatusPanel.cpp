#include "platform/win32/StatusPanel.h"

#include "platform/win32/AppRecordingState.h"

#include <cmath>

namespace ovtr::win32 {

std::wstring exportFormatDisplayText(const ExportFormat format)
{
    return format == ExportFormat::Fbx ? L"fbx" : L"glb";
}

int remainingRecordDelaySeconds(
    const AppRecordingState& state,
    const std::chrono::steady_clock::time_point now
) noexcept
{
    if (!state.recordingDelayActive) {
        return 0;
    }

    const double remainingSeconds = std::chrono::duration<double>(
        state.recordingDelayDeadline - now
    ).count();
    return remainingSeconds > 0.0 ? static_cast<int>(std::ceil(remainingSeconds)) : 0;
}

int remainingRecordDelaySeconds(const AppRecordingState& state) noexcept
{
    return remainingRecordDelaySeconds(state, std::chrono::steady_clock::now());
}

} // namespace ovtr::win32
