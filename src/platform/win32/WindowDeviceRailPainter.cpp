#include "platform/win32/WindowChromePainter.h"

#include "platform/win32/AppDebugUiState.h"
#include "platform/win32/AppProfileState.h"
#include "platform/win32/AppState.h"
#include "platform/win32/Layout.h"
#include "platform/win32/MappingEditPanelPainter.h"
#include "platform/win32/MappingPanelPainter.h"
#include "platform/win32/PaintWidgets.h"
#include "platform/win32/ProfilePanelPainter.h"
#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {
namespace {

constexpr int kTopBarHeight = 32;
constexpr int kDeviceToggleRailWidth = 32;
constexpr int kSplitterWidth = 8;

int activeDebugMonitorHeightForState(const AppDebugUiState& state, const int clientHeight)
{
    if (!state.debugMonitorVisible) {
        return 0;
    }
    return clampDebugMonitorHeightForClient(state.debugMonitorHeight, clientHeight);
}

int leftPanelWidthForState(const AppDebugUiState& state, const int clientWidth)
{
    const int requestedWidth = state.leftPanelWidth > 0 ? state.leftPanelWidth : 0;
    return leftPanelWidthForClient(
        state.devicePanelVisible || state.sessionPanelVisible,
        requestedWidth,
        clientWidth
    );
}

ProfilePanelLayout rightPanelLayoutForState(
    const AppProfileState& state,
    const int clientWidth,
    const int clientHeight,
    const int contentBottom
)
{
    return profilePanelLayoutForClient(
        state.profilePanelVisible || state.mappingPanelVisible || state.editPanelVisible,
        state.profilePanelWidth > 0 ? state.profilePanelWidth : defaultProfilePanelWidthForClient(clientWidth),
        contentBottom,
        clientWidth,
        clientHeight
    );
}

void paintRightPanelFrame(
    HDC drawDc,
    const AppProfileState& state,
    const ProfilePanelLayout& panelLayout
)
{
    UniqueBrush panelBrush(CreateSolidBrush(RGB(20, 23, 28)));
    FillRect(drawDc, &panelLayout.panelRect, panelBrush.get());

    UniquePen panelPen(CreatePen(PS_SOLID, 1, RGB(58, 64, 76)));
    SelectObjectGuard penSelection(drawDc, panelPen.get());
    SelectObjectGuard brushSelection(drawDc, GetStockObject(NULL_BRUSH));
    Rectangle(
        drawDc,
        panelLayout.panelRect.left,
        panelLayout.panelRect.top,
        panelLayout.panelRect.right,
        panelLayout.panelRect.bottom
    );

    UniqueBrush splitterBrush(CreateSolidBrush(
        state.profileSplitterDragging ? RGB(50, 60, 76) : RGB(28, 31, 38)
    ));
    FillRect(drawDc, &panelLayout.splitterRect, splitterBrush.get());

    UniquePen splitterPen(CreatePen(PS_SOLID, 1, RGB(58, 64, 76)));
    {
        SelectObjectGuard splitterPenSelection(drawDc, splitterPen.get());
        MoveToEx(drawDc, panelLayout.splitterRect.left, panelLayout.splitterRect.top, nullptr);
        LineTo(drawDc, panelLayout.splitterRect.left, panelLayout.splitterRect.bottom);
        MoveToEx(drawDc, panelLayout.splitterRect.right - 1, panelLayout.splitterRect.top, nullptr);
        LineTo(drawDc, panelLayout.splitterRect.right - 1, panelLayout.splitterRect.bottom);
    }

    UniquePen handlePen(CreatePen(
        PS_SOLID,
        1,
        state.profileSplitterDragging ? RGB(128, 154, 190) : RGB(84, 92, 108)
    ));
    SelectObjectGuard handlePenSelection(drawDc, handlePen.get());
    const int handleX = panelLayout.splitterRect.left + (kSplitterWidth / 2);
    const int handleTop = panelLayout.splitterRect.top + 20;
    const int handleBottom = panelLayout.splitterRect.bottom > 20
        ? panelLayout.splitterRect.bottom - 20
        : panelLayout.splitterRect.bottom;
    MoveToEx(drawDc, handleX, handleTop, nullptr);
    LineTo(drawDc, handleX, handleBottom);
}

void paintRightRail(
    HDC drawDc,
    HFONT font,
    const AppProfileState& state,
    const int clientWidth,
    const int clientHeight,
    const int contentBottom
)
{
    RECT railRect{clientWidth - kDeviceToggleRailWidth, kTopBarHeight, clientWidth, contentBottom};
    UniqueBrush railBrush(CreateSolidBrush(RGB(20, 23, 28)));
    FillRect(drawDc, &railRect, railBrush.get());

    UniquePen railPen(CreatePen(PS_SOLID, 1, RGB(43, 48, 59)));
    {
        SelectObjectGuard penSelection(drawDc, railPen.get());
        MoveToEx(drawDc, railRect.left, railRect.top, nullptr);
        LineTo(drawDc, railRect.left, railRect.bottom);
    }

    const RECT profileButtonRect = profileToggleButtonRectForClient(contentBottom, clientWidth, clientHeight);
    if (profileButtonRect.right > profileButtonRect.left && profileButtonRect.bottom > profileButtonRect.top) {
        drawProfileToggleButton(drawDc, font, profileButtonRect, state.profilePanelVisible);
    }

    const RECT mappingButtonRect = mappingToggleButtonRectForClient(contentBottom, clientWidth, clientHeight);
    if (mappingButtonRect.right > mappingButtonRect.left && mappingButtonRect.bottom > mappingButtonRect.top) {
        drawMappingToggleButton(drawDc, font, mappingButtonRect, state.mappingPanelVisible);
    }

    const RECT editButtonRect = editToggleButtonRectForClient(contentBottom, clientWidth, clientHeight);
    if (editButtonRect.right > editButtonRect.left && editButtonRect.bottom > editButtonRect.top) {
        drawEditToggleButton(drawDc, font, editButtonRect, state.editPanelVisible);
    }
}

} // namespace

void paintDeviceRailAndSplitter(
    HDC drawDc,
    HFONT font,
    const AppDebugUiState& state,
    const int clientWidth,
    const int clientHeight,
    const int contentBottom
)
{
    RECT railRect{0, kTopBarHeight, kDeviceToggleRailWidth, contentBottom};
    UniqueBrush railBrush(CreateSolidBrush(RGB(20, 23, 28)));
    FillRect(drawDc, &railRect, railBrush.get());

    UniquePen railPen(CreatePen(PS_SOLID, 1, RGB(43, 48, 59)));
    {
        SelectObjectGuard penSelection(drawDc, railPen.get());
        MoveToEx(drawDc, kDeviceToggleRailWidth - 1, railRect.top, nullptr);
        LineTo(drawDc, kDeviceToggleRailWidth - 1, railRect.bottom);
    }

    const RECT deviceButtonRect = deviceToggleButtonRectForClient(contentBottom, clientWidth, clientHeight);
    if (deviceButtonRect.right > deviceButtonRect.left && deviceButtonRect.bottom > deviceButtonRect.top) {
        drawDeviceToggleButton(
            drawDc,
            font,
            deviceButtonRect,
            state.devicePanelVisible
        );
    }

    const RECT sessionButtonRect = sessionToggleButtonRectForClient(contentBottom, clientWidth, clientHeight);
    if (sessionButtonRect.right > sessionButtonRect.left && sessionButtonRect.bottom > sessionButtonRect.top) {
        drawSessionToggleButton(
            drawDc,
            font,
            sessionButtonRect,
            state.sessionPanelVisible
        );
    }

    const RECT splitterRect = splitterRectForClient(
        leftPanelWidthForState(state, clientWidth),
        activeDebugMonitorHeightForState(state, clientHeight),
        clientHeight
    );
    if (splitterRect.bottom <= splitterRect.top || splitterRect.right <= splitterRect.left) {
        return;
    }

    UniqueBrush splitterBrush(CreateSolidBrush(
        state.splitterDragging ? RGB(50, 60, 76) : RGB(28, 31, 38)
    ));
    FillRect(drawDc, &splitterRect, splitterBrush.get());

    UniquePen splitterPen(CreatePen(PS_SOLID, 1, RGB(58, 64, 76)));
    {
        SelectObjectGuard penSelection(drawDc, splitterPen.get());
        MoveToEx(drawDc, splitterRect.left, splitterRect.top, nullptr);
        LineTo(drawDc, splitterRect.left, splitterRect.bottom);
        MoveToEx(drawDc, splitterRect.right - 1, splitterRect.top, nullptr);
        LineTo(drawDc, splitterRect.right - 1, splitterRect.bottom);
    }

    if (!state.devicePanelVisible && !state.sessionPanelVisible) {
        return;
    }

    UniquePen handlePen(CreatePen(
        PS_SOLID,
        1,
        state.splitterDragging ? RGB(128, 154, 190) : RGB(84, 92, 108)
    ));
    SelectObjectGuard penSelection(drawDc, handlePen.get());
    const int handleX = splitterRect.left + (kSplitterWidth / 2);
    const int handleTop = splitterRect.top + 20;
    const int handleBottom = splitterRect.bottom > 20 ? splitterRect.bottom - 20 : splitterRect.bottom;
    MoveToEx(drawDc, handleX, handleTop, nullptr);
    LineTo(drawDc, handleX, handleBottom);
}

void paintProfileRailAndPanel(
    HDC drawDc,
    HFONT font,
    const AppProfileState& state,
    const int clientWidth,
    const int clientHeight,
    const int contentBottom
)
{
    if (clientWidth <= 0 || contentBottom <= kTopBarHeight) {
        return;
    }

    const ProfilePanelLayout panelLayout = profilePanelLayoutForClient(
        state.profilePanelVisible,
        state.profilePanelWidth > 0 ? state.profilePanelWidth : defaultProfilePanelWidthForClient(clientWidth),
        contentBottom,
        clientWidth,
        clientHeight
    );
    if (panelLayout.valid) {
        paintRightPanelFrame(drawDc, state, panelLayout);
        paintProfilePanelContent(drawDc, font, state, panelLayout);
    }

    paintRightRail(drawDc, font, state, clientWidth, clientHeight, contentBottom);
}

void paintProfileRailAndPanel(
    HDC drawDc,
    HFONT font,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const int contentBottom
)
{
    if (clientWidth <= 0 || contentBottom <= kTopBarHeight) {
        return;
    }

    const ProfilePanelLayout panelLayout =
        rightPanelLayoutForState(state, clientWidth, clientHeight, contentBottom);
    if (panelLayout.valid) {
        paintRightPanelFrame(drawDc, state, panelLayout);
        if (state.profilePanelVisible) {
            paintProfilePanelContent(drawDc, font, state, panelLayout);
        } else if (state.mappingPanelVisible) {
            paintMappingPanelContent(drawDc, font, state, panelLayout);
        } else if (state.editPanelVisible) {
            paintMappingEditPanelContent(drawDc, font, state, panelLayout);
        }
    }

    paintRightRail(drawDc, font, state, clientWidth, clientHeight, contentBottom);
}

} // namespace ovtr::win32
