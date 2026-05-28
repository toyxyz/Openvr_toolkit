#include "platform/win32/DebugPanelLinePainter.h"

namespace ovtr::win32 {
namespace {

constexpr int kDebugPanelLineHeight = 18;

void paintDebugLines(
    HDC drawDc,
    const std::vector<std::wstring>& lines,
    const RECT& bodyRect,
    const int textRight,
    const int firstLineIndex,
    const int lastLineIndex
)
{
    int y = bodyRect.top;
    for (int i = firstLineIndex; i < lastLineIndex; ++i) {
        if (i < 0 || i >= static_cast<int>(lines.size())) {
            continue;
        }
        if (y + kDebugPanelLineHeight > bodyRect.bottom) {
            break;
        }
        const std::wstring& line = lines[static_cast<std::size_t>(i)];
        RECT lineRect{bodyRect.left, y, textRight, y + kDebugPanelLineHeight};
        DrawTextW(
            drawDc,
            line.c_str(),
            static_cast<int>(line.size()),
            &lineRect,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
        );
        y += kDebugPanelLineHeight;
    }
}

} // namespace

void paintDebugInfoLines(
    HDC drawDc,
    HFONT bodyFont,
    const std::vector<std::wstring>& debugLines,
    const RECT& bodyRect,
    const int textRight,
    const int firstLineIndex,
    const int lastLineIndex
)
{
    SelectObject(drawDc, bodyFont);
    SetTextColor(drawDc, RGB(176, 185, 198));
    paintDebugLines(drawDc, debugLines, bodyRect, textRight, firstLineIndex, lastLineIndex);
}

void paintDebugMessageLines(
    HDC drawDc,
    HFONT bodyFont,
    const std::vector<std::wstring>& messageLines,
    const RECT& bodyRect,
    const int textRight,
    const int firstLineIndex,
    const int lastLineIndex
)
{
    SelectObject(drawDc, bodyFont);
    SetTextColor(drawDc, RGB(176, 185, 198));
    if (messageLines.empty()) {
        const std::wstring emptyMessage = L"No debug messages yet.";
        RECT lineRect{bodyRect.left, bodyRect.top, textRight, bodyRect.top + kDebugPanelLineHeight};
        DrawTextW(
            drawDc,
            emptyMessage.c_str(),
            static_cast<int>(emptyMessage.size()),
            &lineRect,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
        );
        return;
    }

    paintDebugLines(drawDc, messageLines, bodyRect, textRight, firstLineIndex, lastLineIndex);
}

} // namespace ovtr::win32
