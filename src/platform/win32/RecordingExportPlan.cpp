#include "platform/win32/RecordingExportPlan.h"

#include "platform/win32/AppConfig.h"
#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppRecordingState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/RecordingSessionActions.h"
#include "platform/win32/Win32String.h"

#include <cwctype>
#include <utility>

namespace ovtr::win32 {
namespace {

bool isReservedWindowsDeviceName(std::wstring name)
{
    const std::size_t dot = name.find(L'.');
    if (dot != std::wstring::npos) {
        name.resize(dot);
    }
    for (wchar_t& ch : name) {
        ch = static_cast<wchar_t>(std::towupper(ch));
    }
    return name == L"CON" || name == L"PRN" || name == L"AUX" || name == L"NUL" ||
        name == L"COM1" || name == L"COM2" || name == L"COM3" || name == L"COM4" ||
        name == L"COM5" || name == L"COM6" || name == L"COM7" || name == L"COM8" ||
        name == L"COM9" || name == L"LPT1" || name == L"LPT2" || name == L"LPT3" ||
        name == L"LPT4" || name == L"LPT5" || name == L"LPT6" || name == L"LPT7" ||
        name == L"LPT8" || name == L"LPT9";
}

bool isInvalidWindowsFileNameChar(const wchar_t ch) noexcept
{
    return ch < 32 || ch == L'<' || ch == L'>' || ch == L':' || ch == L'"' ||
        ch == L'/' || ch == L'\\' || ch == L'|' || ch == L'?' || ch == L'*';
}

} // namespace

RecordingExportPlan makeRecordingExportPlan(
    const AppRecordingState& recordingState,
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState
)
{
    return makeRecordingExportPlan(recordingState, runtimeState, deviceState, {});
}

RecordingExportPlan makeRecordingExportPlan(
    const AppRecordingState& recordingState,
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState,
    const std::wstring& sessionName
)
{
    RecordingExportPlan plan;
    plan.session = prepareSessionForExport(
        recordingState.recorder.session(),
        recordingState.currentSessionFolder,
        runtimeState.devices
    );
    applyCustomNamesToExportDevices(deviceState, plan.session.devices);
    plan.exportDirectory = sessionExportDirectoryPath(recordingState.exportDirectory, sessionName);
    plan.exportSampleRate = static_cast<double>(
        sanitizedExportSampleRate(recordingState.recordExportSampleRate, kDefaultRecordExportSampleRate)
    );
    return plan;
}

std::wstring sanitizedSessionFolderName(std::wstring sessionName)
{
    sessionName = trimWide(std::move(sessionName));
    for (wchar_t& ch : sessionName) {
        if (isInvalidWindowsFileNameChar(ch)) {
            ch = L'_';
        }
    }
    while (!sessionName.empty() && (sessionName.back() == L'.' || std::iswspace(sessionName.back()))) {
        sessionName.pop_back();
    }
    if (!sessionName.empty() && isReservedWindowsDeviceName(sessionName)) {
        sessionName.insert(sessionName.begin(), L'_');
    }
    return sessionName;
}

std::filesystem::path sessionExportDirectoryPath(
    const std::filesystem::path& exportDirectory,
    const std::wstring& sessionName
)
{
    const std::filesystem::path baseDirectory = normalizedExportDirectoryPath(exportDirectory);
    const std::wstring folderName = sanitizedSessionFolderName(sessionName);
    if (folderName.empty()) {
        return baseDirectory;
    }
    return (baseDirectory / folderName).lexically_normal();
}

} // namespace ovtr::win32
