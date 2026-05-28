#include "platform/win32/ViewportControlPainter.h"

#include "platform/win32/AppImportedSceneState.h"
#include "platform/win32/AppRecordingState.h"
#include "platform/win32/AppViewportState.h"
#include "platform/win32/RecordingStateQueries.h"
#include "platform/win32/ViewportAnimationControlPainter.h"
#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {
namespace {

void drawViewportIconButton(
    HDC drawDc,
    const RECT& rect,
    const bool active
)
{
    UniqueBrush buttonBrush(CreateSolidBrush(active ? RGB(58, 42, 42) : RGB(30, 34, 42)));
    UniquePen buttonPen(CreatePen(PS_SOLID, 1, active ? RGB(180, 72, 72) : RGB(67, 74, 88)));
    SelectObjectGuard brushSelection(drawDc, buttonBrush.get());
    SelectObjectGuard penSelection(drawDc, buttonPen.get());
    RoundRect(drawDc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);
}

void drawViewportRecordButton(HDC drawDc, const RECT& rect, const bool active)
{
    drawViewportIconButton(drawDc, rect, active);

    const int inset = active ? 10 : 11;
    RECT dotRect{
        rect.left + inset,
        rect.top + inset,
        rect.right - inset,
        rect.bottom - inset
    };
    UniqueBrush dotBrush(CreateSolidBrush(active ? RGB(255, 88, 82) : RGB(228, 68, 64)));
    SelectObjectGuard brushSelection(drawDc, dotBrush.get());
    SelectObjectGuard penSelection(drawDc, GetStockObject(NULL_PEN));
    Ellipse(drawDc, dotRect.left, dotRect.top, dotRect.right, dotRect.bottom);
}

void drawViewportQuadButton(HDC drawDc, const RECT& rect, const bool active)
{
    drawViewportIconButton(drawDc, rect, active);

    constexpr int inset = 11;
    constexpr int gap = 3;
    const int cellWidth = (rect.right - rect.left - inset * 2 - gap) / 2;
    const int cellHeight = (rect.bottom - rect.top - inset * 2 - gap) / 2;
    UniqueBrush cellBrush(CreateSolidBrush(active ? RGB(88, 128, 255) : RGB(154, 166, 188)));
    SelectObjectGuard brushSelection(drawDc, cellBrush.get());
    SelectObjectGuard penSelection(drawDc, GetStockObject(NULL_PEN));
    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 2; ++col) {
            const int left = rect.left + inset + col * (cellWidth + gap);
            const int top = rect.top + inset + row * (cellHeight + gap);
            Rectangle(drawDc, left, top, left + cellWidth, top + cellHeight);
        }
    }
}

} // namespace

void drawViewportControlBar(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppRecordingState& recordingState,
    const AppImportedSceneState& importedSceneState,
    const AppViewportState& viewportState
)
{
    if (!layout.valid) {
        return;
    }

    drawImportedAnimationControls(drawDc, font, layout, importedSceneState);

    UniqueBrush barBrush(CreateSolidBrush(RGB(18, 20, 24)));
    FillRect(drawDc, &layout.barRect, barBrush.get());

    UniquePen borderPen(CreatePen(PS_SOLID, 1, RGB(54, 58, 66)));
    {
        SelectObjectGuard penSelection(drawDc, borderPen.get());
        MoveToEx(drawDc, layout.barRect.left, layout.barRect.top, nullptr);
        LineTo(drawDc, layout.barRect.right, layout.barRect.top);
    }

    drawViewportQuadButton(drawDc, layout.quadViewButtonRect, viewportState.quadViewEnabled);
    drawViewportRecordButton(drawDc, layout.recordButtonRect, isRecordingControlActive(recordingState));

    if (font) {
        SelectObject(drawDc, font);
    }
}

} // namespace ovtr::win32
