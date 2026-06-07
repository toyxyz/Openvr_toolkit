#include "platform/win32/WindowPaintScene.h"

#include "platform/win32/AppConfig.h"
#include "platform/win32/AppState.h"
#include "platform/win32/DebugPanelPainter.h"
#include "platform/win32/DevicePanelPainter.h"
#include "platform/win32/Layout.h"
#include "platform/win32/MarkerPanelPainter.h"
#include "platform/win32/OriginPanelPainter.h"
#include "platform/win32/RecordingSessionList.h"
#include "platform/win32/SessionPanelPainter.h"
#include "platform/win32/StatusBarPainter.h"
#include "platform/win32/StreamingPanelPainter.h"
#include "platform/win32/ViewportControlPainter.h"
#include "platform/win32/WindowChromePainter.h"
#include "platform/win32/WindowLayout.h"

#include <vector>

namespace ovtr::win32 {
namespace {

constexpr int kStatusBarHeight = 36;

} // namespace

void paintWindowScene(
    HDC drawDc,
    AppWindowState* state,
    const WindowPaintFonts& fonts,
    const int clientWidth,
    const int clientHeight
)
{
    const int statusBarTop = clientHeight > kStatusBarHeight ? clientHeight - kStatusBarHeight : 0;
    const int debugMonitorHeight = activeDebugMonitorHeight(state, clientHeight);
    const int debugMonitorTop = statusBarTop > debugMonitorHeight
        ? statusBarTop - debugMonitorHeight
        : statusBarTop;
    const int contentBottom = leftPanelContentBottomForClient(state, clientHeight);
    const OriginPanelLayout originPanelLayout = state
        ? originPanelLayoutForClient(state, clientWidth, clientHeight)
        : OriginPanelLayout{};
    const DeviceListLayout deviceListLayout = state
        ? deviceListLayoutForClient(state, clientWidth, clientHeight)
        : DeviceListLayout{};
    const std::vector<RecordingSessionListRow> sessionRows = state
        ? listRecordingSessionFolders(activeSessionDirectoryPath(*state))
        : std::vector<RecordingSessionListRow>{};
    const SessionListLayout sessionListLayout = state
        ? sessionListLayoutForClient(state, clientWidth, clientHeight, static_cast<int>(sessionRows.size()))
        : SessionListLayout{};
    const StreamingPanelLayout streamingPanelLayout = state
        ? streamingPanelLayoutForClient(state, clientWidth, clientHeight)
        : StreamingPanelLayout{};
    const MarkerListLayout markerListLayout = state
        ? markerListLayoutForClient(state, clientWidth, clientHeight)
        : MarkerListLayout{};
    const ViewportControlLayout viewportControlLayout = state
        ? viewportControlLayoutForClient(state, clientWidth, clientHeight)
        : ViewportControlLayout{};

    paintTopBar(drawDc, fonts.statusFont(), state, clientWidth, clientHeight);

    if (state) {
        paintDeviceListPanel(drawDc, fonts.bodyFont(), fonts.statusFont(), *state, deviceListLayout);
        paintSessionListPanel(
            drawDc,
            fonts.bodyFont(),
            fonts.statusFont(),
            *state,
            sessionListLayout,
            sessionRows
        );
        paintStreamingPanel(
            drawDc,
            fonts.bodyFont(),
            fonts.statusFont(),
            *state,
            streamingPanelLayout
        );
        paintMarkerListPanel(drawDc, fonts.bodyFont(), fonts.statusFont(), *state, markerListLayout);
        paintOriginPanel(
            drawDc,
            fonts.statusFont(),
            fonts.debugFont(),
            originPanelLayout,
            *state
        );
        paintDeviceRailAndSplitter(
            drawDc,
            fonts.statusFont(),
            *state,
            clientWidth,
            clientHeight,
            contentBottom
        );
        paintProfileRailAndPanel(
            drawDc,
            fonts.statusFont(),
            *state,
            clientWidth,
            clientHeight,
            contentBottom
        );
        drawViewportControlBar(
            drawDc,
            fonts.statusFont(),
            viewportControlLayout,
            *state
        );
    }

    if (state && debugMonitorHeight > 0) {
        paintDebugMonitorPanel(
            drawDc,
            fonts.statusFont(),
            fonts.debugOrStatusFont(),
            *state,
            clientWidth,
            clientHeight,
            debugMonitorTop,
            statusBarTop
        );
    }

    paintStatusBar(drawDc, fonts.statusFont(), state, clientWidth, clientHeight, statusBarTop);
}

} // namespace ovtr::win32
