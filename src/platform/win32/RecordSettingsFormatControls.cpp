#include "platform/win32/RecordSettingsCaptureControlParts.h"
#include "platform/win32/DialogControlHelpers.h"

namespace ovtr::win32 {

void createRecordSaveFormatControls(
    HWND hwnd,
    HFONT font,
    const RecordSettingsDialogResult& result,
    RecordSettingsDialogControls& controls
)
{
    HWND saveFormatLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Save format",
        WS_CHILD | WS_VISIBLE,
        18,
        146,
        140,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    controls.saveFormatCombo = CreateWindowExW(
        0,
        L"COMBOBOX",
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
        18,
        172,
        120,
        120,
        hwnd,
        reinterpret_cast<HMENU>(kRecordSaveFormatControlId),
        nullptr,
        nullptr
    );
    if (controls.saveFormatCombo) {
        SendMessageW(controls.saveFormatCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"glb"));
        SendMessageW(controls.saveFormatCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"fbx"));
        SendMessageW(
            controls.saveFormatCombo,
            CB_SETCURSEL,
            result.saveFormat == ExportFormat::Fbx ? 1 : 0,
            0
        );
    }

    applyControlFont(saveFormatLabel, font);
    applyControlFont(controls.saveFormatCombo, font);
}

} // namespace ovtr::win32
