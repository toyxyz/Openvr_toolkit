#include "platform/win32/DebugPanelContentPainter.h"

#include "platform/win32/AppDebugUiState.h"
#include "platform/win32/DebugPanelContentSections.h"
#include "platform/win32/Layout.h"
#include "platform/win32/StatusPanel.h"

#include <string>

namespace ovtr::win32 {
namespace {

constexpr int kContentMargin = 32;
constexpr int kDebugPanelPaddingTop = 10;

} // namespace

void paintDebugMonitorContent(
    HDC drawDc,
    HFONT titleFont,
    HFONT bodyFont,
    const AppRuntimeState& runtimeState,
    const AppRecordingState& recordingState,
    const AppOriginState& originState,
    const AppImportedSceneState& importedSceneState,
    const AppViewportState& viewportState,
    AppDebugUiState& debugUiState,
    const int clientWidth,
    const int clientHeight,
    const int activeDebugMonitorHeight,
    const int debugMonitorTop
)
{
    const auto debugLines = makeDebugMonitorLines(
        runtimeState,
        recordingState,
        originState,
        importedSceneState,
        viewportState
    );
    const int splitX = clientWidth > 900 ? (clientWidth * 48) / 100 : clientWidth / 2;
    const int leftColumnRight = splitX - 18;
    const RECT debugInfoBodyRect =
        debugInfoRectForClient(activeDebugMonitorHeight, clientWidth, clientHeight);
    const RECT messagesBodyRect =
        debugMessagesRectForClient(activeDebugMonitorHeight, clientWidth, clientHeight);
    const HFONT effectiveBodyFont = bodyFont ? bodyFont : titleFont;
    paintDebugInfoContent(drawDc, effectiveBodyFont, debugUiState, debugLines, debugInfoBodyRect);
    const DebugMessageContentMetrics messageMetrics =
        paintDebugMessagesContent(drawDc, effectiveBodyFont, debugUiState, messagesBodyRect);

    SelectObject(drawDc, titleFont ? titleFont : bodyFont);
    SetTextColor(drawDc, RGB(226, 230, 236));
    RECT debugTitleRect{
        kContentMargin,
        debugMonitorTop + kDebugPanelPaddingTop,
        leftColumnRight,
        debugMonitorTop + kDebugPanelPaddingTop + 22
    };
    DrawTextW(drawDc, L"Debug Monitor", -1, &debugTitleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    const std::wstring messagesTitle = debugMessagesTitle(messageMetrics);
    RECT messagesTitleRect{
        messagesBodyRect.left,
        debugMonitorTop + kDebugPanelPaddingTop,
        clientWidth - kContentMargin,
        debugMonitorTop + kDebugPanelPaddingTop + 22
    };
    DrawTextW(
        drawDc,
        messagesTitle.c_str(),
        static_cast<int>(messagesTitle.size()),
        &messagesTitleRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );
}

} // namespace ovtr::win32
