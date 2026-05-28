#include "platform/win32/DebugPanelContentSections.h"

#include "platform/win32/AppDebugUiState.h"
#include "platform/win32/DebugPanelLinePainter.h"
#include "platform/win32/DebugScrollbarPainter.h"
#include "platform/win32/Layout.h"

namespace ovtr::win32 {

void paintDebugInfoContent(
    HDC drawDc,
    HFONT bodyFont,
    AppDebugUiState& state,
    const std::vector<std::wstring>& debugLines,
    const RECT& bodyRect
)
{
    const int visibleLineCount = visibleDebugLineCountForRect(bodyRect);
    const int totalInfoLines = static_cast<int>(debugLines.size()) > 1
        ? static_cast<int>(debugLines.size()) - 1
        : 0;
    const int maxScrollOffset = maxDebugScrollOffset(totalInfoLines, visibleLineCount);
    state.debugInfoScrollOffset = clampDebugScrollOffset(
        state.debugInfoScrollOffset,
        totalInfoLines,
        visibleLineCount
    );
    const int firstLineIndex = 1 + state.debugInfoScrollOffset;
    const int lastLineIndex = firstLineIndex + visibleLineCount < static_cast<int>(debugLines.size())
        ? firstLineIndex + visibleLineCount
        : static_cast<int>(debugLines.size());
    const int textRight = maxScrollOffset > 0 ? bodyRect.right - 12 : bodyRect.right;

    paintDebugInfoLines(
        drawDc,
        bodyFont,
        debugLines,
        bodyRect,
        textRight,
        firstLineIndex,
        lastLineIndex
    );

    if (maxScrollOffset > 0 && totalInfoLines > 0) {
        paintDebugScrollbar(
            drawDc,
            bodyRect,
            totalInfoLines,
            visibleLineCount,
            state.debugInfoScrollOffset,
            false
        );
    }
}

DebugMessageContentMetrics paintDebugMessagesContent(
    HDC drawDc,
    HFONT bodyFont,
    AppDebugUiState& state,
    const RECT& bodyRect
)
{
    const int visibleLineCount = visibleDebugLineCountForRect(bodyRect);

    DebugMessageContentMetrics metrics;
    metrics.totalLineCount = static_cast<int>(state.debugLogLines.size());
    const int maxScrollOffset = maxDebugScrollOffset(metrics.totalLineCount, visibleLineCount);
    state.debugLogScrollOffset = clampDebugScrollOffset(
        state.debugLogScrollOffset,
        metrics.totalLineCount,
        visibleLineCount
    );
    metrics.lastLineIndex = metrics.totalLineCount - state.debugLogScrollOffset;
    metrics.firstLineIndex = metrics.lastLineIndex > visibleLineCount
        ? metrics.lastLineIndex - visibleLineCount
        : 0;

    const int textRight = maxScrollOffset > 0 ? bodyRect.right - 12 : bodyRect.right;
    paintDebugMessageLines(
        drawDc,
        bodyFont,
        state.debugLogLines,
        bodyRect,
        textRight,
        metrics.firstLineIndex,
        metrics.lastLineIndex
    );

    if (maxScrollOffset > 0) {
        paintDebugScrollbar(
            drawDc,
            bodyRect,
            metrics.totalLineCount,
            visibleLineCount,
            state.debugLogScrollOffset,
            true
        );
    }
    return metrics;
}

std::wstring debugMessagesTitle(const DebugMessageContentMetrics& metrics)
{
    std::wstring title = L"Messages";
    if (metrics.totalLineCount > 0) {
        title += L"  ";
        title += std::to_wstring(metrics.firstLineIndex + 1);
        title += L"-";
        title += std::to_wstring(metrics.lastLineIndex);
        title += L" / ";
        title += std::to_wstring(metrics.totalLineCount);
    }
    return title;
}

} // namespace ovtr::win32
