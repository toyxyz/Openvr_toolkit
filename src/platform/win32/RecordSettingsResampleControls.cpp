#include "platform/win32/RecordSettingsCaptureControlParts.h"

#include "platform/win32/DialogControlHelpers.h"
#include "platform/win32/Dialogs.h"

namespace ovtr::win32 {

void createRecordResampleControls(
    HWND hwnd,
    HFONT font,
    const RecordSettingsDialogResult& result,
    RecordSettingsDialogControls& controls
)
{
    HWND resampleFpsLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Resample FPS",
        WS_CHILD | WS_VISIBLE,
        178,
        146,
        140,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    controls.resampleFpsEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        formatFloatText(result.exportSampleRate).c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        178,
        172,
        96,
        24,
        hwnd,
        reinterpret_cast<HMENU>(kRecordResampleFpsEditControlId),
        nullptr,
        nullptr
    );
    HWND resampleFpsHint = CreateWindowExW(
        0,
        L"STATIC",
        L"fps",
        WS_CHILD | WS_VISIBLE,
        286,
        175,
        42,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );

    applyControlFont(resampleFpsLabel, font);
    applyControlFont(controls.resampleFpsEdit, font);
    applyControlFont(resampleFpsHint, font);
}

} // namespace ovtr::win32
