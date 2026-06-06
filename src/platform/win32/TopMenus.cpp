#include "platform/win32/TopMenus.h"

#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/AppTopBarState.h"
#include "platform/win32/ImportedSceneActions.h"
#include "platform/win32/Menus.h"
#include "platform/win32/OriginDialog.h"
#include "platform/win32/SessionExportActions.h"
#include "platform/win32/SessionSaveActions.h"
#include "platform/win32/TopMenuSettingsActions.h"
#include "platform/win32/Win32MenuResources.h"

#include <array>

namespace ovtr::win32 {
namespace {

constexpr UINT kSettingsMenuColorId = 1101;
constexpr UINT kSettingsMenuOriginId = 1102;
constexpr UINT kSettingsMenuLocationId = 1103;
constexpr UINT kSettingsMenuStreamingId = 1104;
constexpr UINT kFileMenuImportGlbId = 1201;
constexpr UINT kFileMenuExportSessionId = 1202;
constexpr UINT kFileMenuSaveSessionId = 1203;

void setActiveTopBarMenu(HWND hwnd, AppWindowState& state, const ActiveTopBarMenu activeMenu)
{
    state.activeTopBarMenu = activeMenu;
    InvalidateRect(hwnd, nullptr, FALSE);
    UpdateWindow(hwnd);
}

UINT trackTopBarMenu(
    HWND hwnd,
    AppWindowState& state,
    HMENU menu,
    const ActiveTopBarMenu activeMenu,
    const POINT menuPoint
)
{
    SetForegroundWindow(hwnd);
    setActiveTopBarMenu(hwnd, state, activeMenu);
    const UINT command = TrackPopupMenu(
        menu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
        menuPoint.x,
        menuPoint.y,
        0,
        hwnd,
        nullptr
    );
    setActiveTopBarMenu(hwnd, state, ActiveTopBarMenu::None);
    return command;
}

} // namespace

void showTopSettingsMenu(HWND hwnd, AppWindowState& state, const RECT& settingRect)
{
    UniqueMenu menu(CreatePopupMenu());
    if (!menu) {
        return;
    }

    std::array<PopupMenuItem, 4> menuItems{{
        PopupMenuItem{kSettingsMenuColorId, L"Appearance..."},
        PopupMenuItem{kSettingsMenuOriginId, L"Origin..."},
        PopupMenuItem{kSettingsMenuLocationId, L"Record..."},
        PopupMenuItem{kSettingsMenuStreamingId, L"Streaming..."},
    }};
    for (PopupMenuItem& item : menuItems) {
        appendPopupMenuItem(menu.get(), item);
    }

    POINT menuPoint{settingRect.left, settingRect.bottom};
    ClientToScreen(hwnd, &menuPoint);
    const UINT command = trackTopBarMenu(hwnd, state, menu.get(), ActiveTopBarMenu::Setting, menuPoint);
    if (command == kSettingsMenuColorId) {
        appendDebugLog(state, L"Appearance settings opened");
        showViewportColorSettings(hwnd, state);
        InvalidateRect(hwnd, nullptr, FALSE);
    } else if (command == kSettingsMenuOriginId) {
        appendDebugLog(state, L"Origin settings opened");
        showOriginSettings(hwnd, state);
        InvalidateRect(hwnd, nullptr, FALSE);
    } else if (command == kSettingsMenuLocationId) {
        appendDebugLog(state, L"Record settings opened");
        showExportLocationSettings(hwnd, state);
        InvalidateRect(hwnd, nullptr, FALSE);
    } else if (command == kSettingsMenuStreamingId) {
        appendDebugLog(state, L"Streaming settings opened");
        showStreamingSettings(hwnd, state);
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

void showTopFileMenu(HWND hwnd, AppWindowState& state, const RECT& fileRect)
{
    UniqueMenu menu(CreatePopupMenu());
    if (!menu) {
        return;
    }

    std::array<PopupMenuItem, 3> menuItems{{
        PopupMenuItem{kFileMenuImportGlbId, L"Import GLB..."},
        PopupMenuItem{kFileMenuSaveSessionId, L"Save Session"},
        PopupMenuItem{kFileMenuExportSessionId, L"Export Session"},
    }};
    PopupMenuItem& importItem = menuItems[0];
    PopupMenuItem& saveSessionItem = menuItems[1];
    PopupMenuItem& exportSessionItem = menuItems[2];
    appendPopupMenuItem(menu.get(), importItem);
    AppendMenuW(menu.get(), MF_SEPARATOR, 0, nullptr);
    appendPopupMenuItem(menu.get(), saveSessionItem);
    appendPopupMenuItem(menu.get(), exportSessionItem);

    POINT menuPoint{fileRect.left, fileRect.bottom};
    ClientToScreen(hwnd, &menuPoint);
    const UINT command = trackTopBarMenu(hwnd, state, menu.get(), ActiveTopBarMenu::File, menuPoint);
    if (command == kFileMenuImportGlbId) {
        importGlbFromFile(hwnd, state);
    } else if (command == kFileMenuSaveSessionId) {
        saveLoadedSessionFolder(hwnd, state);
    } else if (command == kFileMenuExportSessionId) {
        exportLoadedSession(hwnd, state);
    }
}

} // namespace ovtr::win32
