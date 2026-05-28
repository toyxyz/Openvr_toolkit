#include "platform/win32/RecordSettingsCaptureControlParts.h"

#include "platform/win32/DialogControlHelpers.h"
#include "platform/win32/Dialogs.h"

namespace ovtr::win32 {

void createRecordDelayControls(
    HWND hwnd,
    HFONT font,
    const RecordSettingsDialogResult& result,
    RecordSettingsDialogControls& controls
)
{
    HWND delayLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Record delay (seconds)",
        WS_CHILD | WS_VISIBLE,
        18,
        84,
        160,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    controls.delayEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        formatFloatText(result.recordDelaySeconds).c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        18,
        110,
        120,
        24,
        hwnd,
        reinterpret_cast<HMENU>(kRecordDelayEditControlId),
        nullptr,
        nullptr
    );
    HWND delayHint = CreateWindowExW(
        0,
        L"STATIC",
        L"0 starts immediately",
        WS_CHILD | WS_VISIBLE,
        150,
        113,
        180,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );

    applyControlFont(delayLabel, font);
    applyControlFont(controls.delayEdit, font);
    applyControlFont(delayHint, font);
}

} // namespace ovtr::win32
