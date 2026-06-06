#include "platform/win32/RecordSettingsDialogControlSections.h"
#include "platform/win32/DialogControlHelpers.h"

namespace ovtr::win32 {

void createRecordSettingsExportFolderControls(
    HWND hwnd,
    HFONT font,
    const RecordSettingsDialogResult& result,
    RecordSettingsDialogControls& controls
)
{
    HWND label = CreateWindowExW(
        0,
        L"STATIC",
        L"Export folder",
        WS_CHILD | WS_VISIBLE,
        18,
        18,
        140,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    controls.directoryEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        result.directory.wstring().c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        18,
        44,
        410,
        24,
        hwnd,
        reinterpret_cast<HMENU>(kExportLocationEditControlId),
        nullptr,
        nullptr
    );
    HWND browseButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"Browse...",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        440,
        43,
        100,
        26,
        hwnd,
        reinterpret_cast<HMENU>(kExportLocationBrowseControlId),
        nullptr,
        nullptr
    );
    HWND sessionLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Session folder",
        WS_CHILD | WS_VISIBLE,
        18,
        76,
        140,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    controls.sessionDirectoryEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        result.sessionDirectory.wstring().c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        18,
        102,
        410,
        24,
        hwnd,
        reinterpret_cast<HMENU>(kSessionLocationEditControlId),
        nullptr,
        nullptr
    );
    HWND sessionBrowseButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"Browse...",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        440,
        101,
        100,
        26,
        hwnd,
        reinterpret_cast<HMENU>(kSessionLocationBrowseControlId),
        nullptr,
        nullptr
    );

    applyControlFont(label, font);
    applyControlFont(controls.directoryEdit, font);
    applyControlFont(browseButton, font);
    applyControlFont(sessionLabel, font);
    applyControlFont(controls.sessionDirectoryEdit, font);
    applyControlFont(sessionBrowseButton, font);
}

} // namespace ovtr::win32
