#include "platform/win32/Dialogs.h"

#include "platform/win32/DialogControlHelpers.h"
#include "platform/win32/ModalDialog.h"
#include "platform/win32/RealtimePoseSmoothing.h"

#include <cwchar>

namespace ovtr::win32 {
namespace {

inline constexpr int kDialogWidth = 420;
inline constexpr int kDialogHeight = 250;
inline constexpr UINT_PTR kEnableControlId = 4700;
inline constexpr UINT_PTR kPresetControlId = 4701;
inline constexpr const wchar_t* kDialogClassName = L"OpenVRTrackerRecorderStreamingSettingsDialog";

struct StreamingDialogControls {
    HWND enableCheck = nullptr;
    HWND presetCombo = nullptr;
    HWND paramsText = nullptr;
};

struct StreamingDialogState {
    StreamingSettingsConfig result;
    StreamingDialogControls controls;
    bool accepted = false;
    bool done = false;
};

RealtimeSmoothingPreset presetFromCombo(HWND combo)
{
    const LRESULT selected = SendMessageW(combo, CB_GETCURSEL, 0, 0);
    switch (selected) {
    case 0:
        return RealtimeSmoothingPreset::VeryLight;
    case 1:
        return RealtimeSmoothingPreset::Light;
    case 3:
        return RealtimeSmoothingPreset::Strong;
    case 4:
        return RealtimeSmoothingPreset::VeryStrong;
    case 2:
    default:
        return RealtimeSmoothingPreset::Normal;
    }
}

int presetComboIndex(const RealtimeSmoothingPreset preset) noexcept
{
    switch (preset) {
    case RealtimeSmoothingPreset::VeryLight:
        return 0;
    case RealtimeSmoothingPreset::Light:
        return 1;
    case RealtimeSmoothingPreset::Strong:
        return 3;
    case RealtimeSmoothingPreset::VeryStrong:
        return 4;
    case RealtimeSmoothingPreset::Normal:
    default:
        return 2;
    }
}

void updateParamsText(const StreamingDialogControls& controls)
{
    if (!controls.paramsText || !controls.presetCombo) {
        return;
    }
    const OneEuroPresetParameters params = oneEuroParametersForPreset(presetFromCombo(controls.presetCombo));
    wchar_t text[160]{};
    swprintf_s(
        text,
        L"min cutoff %.2f Hz    beta %.2f    derivative cutoff %.2f Hz",
        params.minCutoffHz,
        params.beta,
        params.derivativeCutoffHz
    );
    SetWindowTextW(controls.paramsText, text);
}

void createStreamingDialogControls(HWND hwnd, StreamingDialogState& dialog)
{
    HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    dialog.controls.enableCheck = CreateWindowExW(
        0,
        L"BUTTON",
        L"Enable realtime smoothing",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        24,
        28,
        240,
        24,
        hwnd,
        reinterpret_cast<HMENU>(kEnableControlId),
        nullptr,
        nullptr
    );
    SendMessageW(
        dialog.controls.enableCheck,
        BM_SETCHECK,
        dialog.result.realtimeSmoothingEnabled ? BST_CHECKED : BST_UNCHECKED,
        0
    );

    HWND presetLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"One Euro preset",
        WS_CHILD | WS_VISIBLE,
        24,
        72,
        120,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    dialog.controls.presetCombo = CreateWindowExW(
        0,
        L"COMBOBOX",
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
        152,
        68,
        130,
        120,
        hwnd,
        reinterpret_cast<HMENU>(kPresetControlId),
        nullptr,
        nullptr
    );
    constexpr const wchar_t* labels[] = {L"Very Light", L"Light", L"Normal", L"Strong", L"Very Strong"};
    for (const wchar_t* label : labels) {
        SendMessageW(dialog.controls.presetCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
    }
    SendMessageW(
        dialog.controls.presetCombo,
        CB_SETCURSEL,
        static_cast<WPARAM>(presetComboIndex(dialog.result.realtimeSmoothingPreset)),
        0
    );

    dialog.controls.paramsText = CreateWindowExW(
        0,
        L"STATIC",
        L"",
        WS_CHILD | WS_VISIBLE,
        24,
        112,
        350,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    HWND okButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"OK",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        208,
        158,
        90,
        28,
        hwnd,
        reinterpret_cast<HMENU>(IDOK),
        nullptr,
        nullptr
    );
    HWND cancelButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"Cancel",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        308,
        158,
        90,
        28,
        hwnd,
        reinterpret_cast<HMENU>(IDCANCEL),
        nullptr,
        nullptr
    );

    applyControlFont(dialog.controls.enableCheck, font);
    applyControlFont(presetLabel, font);
    applyControlFont(dialog.controls.presetCombo, font);
    applyControlFont(dialog.controls.paramsText, font);
    applyControlFont(okButton, font);
    applyControlFont(cancelButton, font);
    updateParamsText(dialog.controls);
}

void finishStreamingDialog(HWND hwnd, StreamingDialogState& dialog, const bool accepted)
{
    if (accepted) {
        dialog.result.realtimeSmoothingEnabled = SendMessageW(
            dialog.controls.enableCheck,
            BM_GETCHECK,
            0,
            0
        ) == BST_CHECKED;
        dialog.result.realtimeSmoothingPreset = presetFromCombo(dialog.controls.presetCombo);
    }
    dialog.accepted = accepted;
    dialog.done = true;
    DestroyWindow(hwnd);
}

LRESULT CALLBACK streamingSettingsDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto* dialog = reinterpret_cast<StreamingDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (message) {
    case WM_NCCREATE: {
        const auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lparam);
        dialog = reinterpret_cast<StreamingDialogState*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog));
        return TRUE;
    }
    case WM_CREATE:
        createStreamingDialogControls(hwnd, *dialog);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wparam) == kPresetControlId && HIWORD(wparam) == CBN_SELCHANGE) {
            updateParamsText(dialog->controls);
            return 0;
        }
        if (LOWORD(wparam) == IDOK) {
            finishStreamingDialog(hwnd, *dialog, true);
            return 0;
        }
        if (LOWORD(wparam) == IDCANCEL) {
            finishStreamingDialog(hwnd, *dialog, false);
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

bool promptForStreamingSettings(
    HWND parent,
    const StreamingSettingsConfig& input,
    StreamingSettingsConfig& outResult
)
{
    StreamingDialogState dialog;
    dialog.result = input;

    ModalDialogHost host(parent);
    const ModalDialogDescriptor descriptor{kDialogClassName, L"Streaming Settings", kDialogWidth, kDialogHeight};
    HWND dialogWindow = host.create(descriptor, &dialog);
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
