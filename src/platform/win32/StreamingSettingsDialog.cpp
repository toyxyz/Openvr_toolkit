#include "platform/win32/Dialogs.h"

#include "platform/win32/DialogControlHelpers.h"
#include "platform/win32/ModalDialog.h"
#include "platform/win32/RealtimePoseSmoothing.h"

#include <cwchar>
#include <cwctype>
#include <initializer_list>
#include <iterator>

namespace ovtr::win32 {
namespace {

inline constexpr int kDialogWidth = 440;
inline constexpr int kDialogHeight = 300;
inline constexpr UINT_PTR kEnableControlId = 4700;
inline constexpr UINT_PTR kPresetControlId = 4701;
inline constexpr UINT_PTR kVmcPortControlId = 4702;
inline constexpr UINT_PTR kVmcEnableControlId = 4703;
inline constexpr const wchar_t* kDialogClassName = L"OpenVRTrackerRecorderStreamingSettingsDialog";

struct StreamingDialogControls {
    HWND enableCheck = nullptr;
    HWND presetCombo = nullptr;
    HWND paramsText = nullptr;
    HWND vmcPortEdit = nullptr;
    HWND vmcEnableCheck = nullptr;
};

struct StreamingDialogState {
    StreamingSettingsConfig result;
    StreamingDialogControls controls;
    bool accepted = false;
    bool done = false;
};

RealtimeSmoothingPreset presetFromCombo(HWND combo)
{
    switch (SendMessageW(combo, CB_GETCURSEL, 0, 0)) {
    case 0: return RealtimeSmoothingPreset::VeryLight;
    case 1: return RealtimeSmoothingPreset::Light;
    case 3: return RealtimeSmoothingPreset::Strong;
    case 4: return RealtimeSmoothingPreset::VeryStrong;
    case 2:
    default: return RealtimeSmoothingPreset::Normal;
    }
}

int presetComboIndex(const RealtimeSmoothingPreset preset) noexcept
{
    switch (preset) {
    case RealtimeSmoothingPreset::VeryLight: return 0;
    case RealtimeSmoothingPreset::Light: return 1;
    case RealtimeSmoothingPreset::Strong: return 3;
    case RealtimeSmoothingPreset::VeryStrong: return 4;
    case RealtimeSmoothingPreset::Normal:
    default: return 2;
    }
}

HWND makeControl(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y, int w, int h, UINT_PTR id = 0)
{
    return CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h, parent,
        reinterpret_cast<HMENU>(id), nullptr, nullptr);
}

void updateParamsText(const StreamingDialogControls& controls)
{
    const OneEuroPresetParameters params = oneEuroParametersForPreset(presetFromCombo(controls.presetCombo));
    wchar_t text[160]{};
    swprintf_s(text, L"min cutoff %.2f Hz    beta %.2f    derivative cutoff %.2f Hz",
        params.minCutoffHz, params.beta, params.derivativeCutoffHz);
    SetWindowTextW(controls.paramsText, text);
}

void setPortText(HWND edit, const int port)
{
    wchar_t text[16]{};
    swprintf_s(text, L"%d", port);
    SetWindowTextW(edit, text);
}

bool readPortText(HWND edit, int& outPort)
{
    wchar_t text[32]{};
    GetWindowTextW(edit, text, static_cast<int>(std::size(text)));
    wchar_t* end = nullptr;
    const long value = std::wcstol(text, &end, 10);
    while (end && *end != 0 && std::iswspace(*end)) {
        ++end;
    }
    if (text[0] == 0 || (end && *end != 0) || value < 1 || value > 65535) {
        return false;
    }
    outPort = static_cast<int>(value);
    return true;
}

void applyFonts(const StreamingDialogControls& controls, HWND hwnd)
{
    HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    EnumChildWindows(hwnd, [](HWND child, LPARAM param) -> BOOL {
        SendMessageW(child, WM_SETFONT, static_cast<WPARAM>(param), TRUE);
        return TRUE;
    }, reinterpret_cast<LPARAM>(font));
    applyControlFont(controls.enableCheck, font);
}

void createStreamingDialogControls(HWND hwnd, StreamingDialogState& dialog)
{
    dialog.controls.enableCheck = makeControl(hwnd, L"BUTTON", L"Enable realtime smoothing",
        WS_TABSTOP | BS_AUTOCHECKBOX, 24, 28, 240, 24, kEnableControlId);
    SendMessageW(dialog.controls.enableCheck, BM_SETCHECK,
        dialog.result.realtimeSmoothingEnabled ? BST_CHECKED : BST_UNCHECKED, 0);

    makeControl(hwnd, L"STATIC", L"One Euro preset", 0, 24, 72, 120, 20);
    dialog.controls.presetCombo = makeControl(hwnd, L"COMBOBOX", L"",
        WS_TABSTOP | CBS_DROPDOWNLIST, 152, 68, 140, 130, kPresetControlId);
    for (const wchar_t* label : {L"Very Light", L"Light", L"Normal", L"Strong", L"Very Strong"}) {
        SendMessageW(dialog.controls.presetCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
    }
    SendMessageW(dialog.controls.presetCombo, CB_SETCURSEL,
        static_cast<WPARAM>(presetComboIndex(dialog.result.realtimeSmoothingPreset)), 0);

    dialog.controls.paramsText = makeControl(hwnd, L"STATIC", L"", 0, 24, 112, 380, 20);
    makeControl(hwnd, L"STATIC", L"VMC port", 0, 24, 152, 120, 20);
    dialog.controls.vmcPortEdit = makeControl(hwnd, L"EDIT", L"", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
        152, 148, 80, 24, kVmcPortControlId);
    setPortText(dialog.controls.vmcPortEdit, dialog.result.vmcPort);
    dialog.controls.vmcEnableCheck = makeControl(hwnd, L"BUTTON", L"Enable VMC receive",
        WS_TABSTOP | BS_AUTOCHECKBOX, 252, 148, 170, 24, kVmcEnableControlId);
    SendMessageW(dialog.controls.vmcEnableCheck, BM_SETCHECK,
        dialog.result.vmcReceiveEnabled ? BST_CHECKED : BST_UNCHECKED, 0);

    makeControl(hwnd, L"BUTTON", L"OK", WS_TABSTOP | BS_DEFPUSHBUTTON, 228, 218, 90, 28, IDOK);
    makeControl(hwnd, L"BUTTON", L"Cancel", WS_TABSTOP, 328, 218, 90, 28, IDCANCEL);
    applyFonts(dialog.controls, hwnd);
    updateParamsText(dialog.controls);
}

void finishStreamingDialog(HWND hwnd, StreamingDialogState& dialog, const bool accepted)
{
    if (accepted) {
        int port = 0;
        if (!readPortText(dialog.controls.vmcPortEdit, port)) {
            MessageBoxW(hwnd, L"VMC port must be between 1 and 65535.", L"Streaming Settings", MB_OK | MB_ICONWARNING);
            return;
        }
        dialog.result.realtimeSmoothingEnabled =
            SendMessageW(dialog.controls.enableCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
        dialog.result.realtimeSmoothingPreset = presetFromCombo(dialog.controls.presetCombo);
        dialog.result.vmcReceiveEnabled =
            SendMessageW(dialog.controls.vmcEnableCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
        dialog.result.vmcPort = port;
    }
    dialog.accepted = accepted;
    dialog.done = true;
    DestroyWindow(hwnd);
}

LRESULT CALLBACK streamingSettingsDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto* dialog = reinterpret_cast<StreamingDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (message) {
    case WM_NCCREATE:
        dialog = reinterpret_cast<StreamingDialogState*>(reinterpret_cast<LPCREATESTRUCTW>(lparam)->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog));
        return TRUE;
    case WM_CREATE:
        createStreamingDialogControls(hwnd, *dialog);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wparam) == kPresetControlId && HIWORD(wparam) == CBN_SELCHANGE) {
            updateParamsText(dialog->controls);
            return 0;
        }
        if (LOWORD(wparam) == IDOK || LOWORD(wparam) == IDCANCEL) {
            finishStreamingDialog(hwnd, *dialog, LOWORD(wparam) == IDOK);
            return 0;
        }
        break;
    case WM_CLOSE:
        finishStreamingDialog(hwnd, *dialog, false);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace

bool registerStreamingSettingsDialogClass(HINSTANCE instance)
{
    WNDCLASSW dialogClass{};
    dialogClass.style = CS_HREDRAW | CS_VREDRAW;
    dialogClass.lpfnWndProc = streamingSettingsDialogProc;
    dialogClass.hInstance = instance;
    dialogClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    dialogClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    dialogClass.lpszClassName = kDialogClassName;
    return RegisterClassW(&dialogClass) != 0;
}

bool promptForStreamingSettings(HWND parent, const StreamingSettingsConfig& input, StreamingSettingsConfig& outResult)
{
    StreamingDialogState dialog;
    dialog.result = input;
    ModalDialogHost host(parent);
    HWND dialogWindow = host.create({kDialogClassName, L"Streaming Settings", kDialogWidth, kDialogHeight}, &dialog);
    if (!dialogWindow) {
        return false;
    }
    host.runMessageLoop(dialogWindow, dialog.done);
    host.restoreParent();
    host.repostQuitIfReceived();
    if (!dialog.accepted) {
        return false;
    }
    outResult = dialog.result;
    return true;
}

} // namespace ovtr::win32
