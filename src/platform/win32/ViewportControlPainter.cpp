#include "platform/win32/ViewportControlPainter.h"
#include "platform/win32/ViewportImportedSceneBar.h"
#include "platform/win32/UiTheme.h"

#include "platform/win32/AppImportedSceneState.h"
#include "platform/win32/AppLoadedSessionState.h"
#include "platform/win32/AppRecordingState.h"
#include "platform/win32/AppSessionState.h"
#include "platform/win32/AppStreamingState.h"
#include "platform/win32/AppViewportState.h"
#include "platform/win32/RecordingStateQueries.h"
#include "platform/win32/ViewportAnimationControlPainter.h"
#include "platform/win32/ViewportAnimationControlSections.h"
#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {
namespace {

void drawViewportIconButton(
    HDC drawDc,
    const RECT& rect,
    const bool active
)
{
    const RECT surface{rect.left, rect.top + 7, rect.right, rect.bottom - 7};
    ui::buttonSurface(drawDc, surface, active);
}

void drawViewportRecordButton(HDC drawDc, const RECT& rect, const bool active)
{
    const RECT surface{rect.left, rect.top + 7, rect.right, rect.bottom - 7};
    UniqueBrush surfaceBrush(CreateSolidBrush(active ? ui::recording : ui::button));
    UniquePen surfacePen(CreatePen(PS_SOLID, 1, active ? ui::record : ui::buttonBorder));
    SelectObjectGuard surfaceBrushSelection(drawDc, surfaceBrush.get());
    SelectObjectGuard surfacePenSelection(drawDc, surfacePen.get());
    RoundRect(drawDc, surface.left, surface.top, surface.right, surface.bottom, 5, 5);

    const int inset = active ? 10 : 11;
    RECT dotRect{
        rect.left + inset,
        rect.top + inset,
        rect.right - inset,
        rect.bottom - inset
    };
    UniqueBrush dotBrush(CreateSolidBrush(ui::record));
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
    UniqueBrush cellBrush(CreateSolidBrush(active ? ui::onAccent : ui::muted));
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

void drawViewportShowTextButton(HDC drawDc, HFONT font, const RECT& rect, const bool active)
{
    drawViewportIconButton(drawDc, rect, active);
    if (font) {
        SelectObject(drawDc, font);
    }
    SetBkMode(drawDc, TRANSPARENT);
    SetTextColor(drawDc, active ? ui::onAccent : ui::muted);
    RECT textRect = rect;
    DrawTextW(drawDc, L"Labels", -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void drawViewportShowModelButton(HDC drawDc, const RECT& rect, const bool active)
{
    drawViewportIconButton(drawDc, rect, active);

    UniquePen modelPen(CreatePen(PS_SOLID, 2, active ? ui::onAccent : ui::muted));
    SelectObjectGuard penSelection(drawDc, modelPen.get());
    SelectObjectGuard brushSelection(drawDc, GetStockObject(NULL_BRUSH));
    const int left = rect.left + 14;
    const int top = rect.top + 16;
    const int right = rect.right - 15;
    const int bottom = rect.bottom - 10;
    Rectangle(drawDc, left, top, right, bottom);
    MoveToEx(drawDc, left, top, nullptr);
    LineTo(drawDc, left + 5, top - 5);
    LineTo(drawDc, right + 5, top - 5);
    LineTo(drawDc, right, top);
    MoveToEx(drawDc, right, bottom, nullptr);
    LineTo(drawDc, right + 5, bottom - 5);
    LineTo(drawDc, right + 5, top - 5);
}

void drawViewportSmoothButton(HDC drawDc, HFONT font, const RECT& rect, const bool active)
{
    drawViewportIconButton(drawDc, rect, active);
    if (font) {
        SelectObject(drawDc, font);
    }
    SetBkMode(drawDc, TRANSPARENT);
    SetTextColor(drawDc, active ? ui::onAccent : ui::muted);
    RECT textRect = rect;
    DrawTextW(drawDc, L"Smooth", -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void drawViewportLockButton(HDC drawDc, HFONT font, const RECT& rect, const bool active)
{
    drawViewportIconButton(drawDc, rect, active);
    if (font) {
        SelectObject(drawDc, font);
    }
    SetBkMode(drawDc, TRANSPARENT);
    SetTextColor(drawDc, active ? ui::onAccent : ui::muted);
    RECT textRect = rect;
    DrawTextW(drawDc, L"Lock", -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void drawViewportSessionBox(HDC drawDc, HFONT font, const ViewportControlLayout& layout, const AppSessionState& state)
{
    if (layout.sessionBoxRect.right <= layout.sessionBoxRect.left) {
        return;
    }

    UniqueBrush boxBrush(CreateSolidBrush(ui::panel));
    FillRect(drawDc, &layout.sessionBoxRect, boxBrush.get());
    UniquePen boxPen(CreatePen(PS_SOLID, 1, ui::buttonBorder));
    {
        SelectObjectGuard penSelection(drawDc, boxPen.get());
        SelectObjectGuard brushSelection(drawDc, GetStockObject(NULL_BRUSH));
        Rectangle(
            drawDc,
            layout.sessionBoxRect.left,
            layout.sessionBoxRect.top,
            layout.sessionBoxRect.right,
            layout.sessionBoxRect.bottom
        );
    }

    if (font) {
        SelectObject(drawDc, font);
    }
    SetBkMode(drawDc, TRANSPARENT);
    SetTextColor(drawDc, ui::muted);
    RECT labelRect = layout.sessionLabelRect;
    DrawTextW(drawDc, L"Session", -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(drawDc, ui::text);
    RECT valueRect = layout.sessionValueRect;
    DrawTextW(
        drawDc,
        state.sessionName.c_str(),
        -1,
        &valueRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );
}

} // namespace

void drawViewportControlBar(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppRecordingState& recordingState,
    const AppImportedSceneState& importedSceneState,
    const AppLoadedSessionState& loadedSessionState,
    const AppSessionState& sessionState,
    const AppStreamingState& streamingState,
    const AppViewportState& viewportState,
    const bool trackedDevicesVisible
)
{
    if (!layout.valid) {
        return;
    }
    SelectObjectGuard controlFont(drawDc, font);
    drawImportedSceneInfoBar(drawDc, font, layout, importedSceneState);

    if (loadedSessionState.loadedSessionActive && layout.animationValid) {
        UniqueBrush animationBrush(CreateSolidBrush(ui::panel));
        FillRect(drawDc, &layout.animationBarRect, animationBrush.get());
        UniquePen animationBorder(CreatePen(PS_SOLID, 1, ui::border));
        {
            SelectObjectGuard penSelection(drawDc, animationBorder.get());
            MoveToEx(drawDc, layout.animationBarRect.left, layout.animationBarRect.top, nullptr);
            LineTo(drawDc, layout.animationBarRect.right, layout.animationBarRect.top);
        }
        drawImportedAnimationButtons(drawDc, font, layout, loadedSessionState.loadedSessionPlaying);
        drawCombinedAnimationTimeline(drawDc, layout, importedSceneState, loadedSessionState);
        drawCombinedAnimationFrameText(drawDc, font, layout, importedSceneState, loadedSessionState);
    } else {
        drawImportedAnimationControls(drawDc, font, layout, importedSceneState);
    }

    UniqueBrush barBrush(CreateSolidBrush(ui::toolbar));
    FillRect(drawDc, &layout.barRect, barBrush.get());

    UniquePen borderPen(CreatePen(PS_SOLID, 1, ui::border));
    {
        SelectObjectGuard penSelection(drawDc, borderPen.get());
        MoveToEx(drawDc, layout.barRect.left, layout.barRect.top, nullptr);
        LineTo(drawDc, layout.barRect.right, layout.barRect.top);
    }

    drawViewportQuadButton(drawDc, layout.quadViewButtonRect, viewportState.quadViewEnabled);
    drawViewportShowTextButton(drawDc, font, layout.showTextButtonRect, viewportState.deviceLabelsVisible);
    drawViewportShowModelButton(drawDc, layout.showModelButtonRect, trackedDevicesVisible);
    drawViewportSmoothButton(drawDc, font, layout.smoothButtonRect, streamingState.realtimeSmoothingEnabled);
    drawViewportLockButton(drawDc, font, layout.lockButtonRect, viewportState.cameraDeviceLockEnabled);
    drawViewportRecordButton(drawDc, layout.recordButtonRect, isRecordingControlActive(recordingState));
    drawViewportSessionBox(drawDc, font, layout, sessionState);

    if (font) {
        SelectObject(drawDc, font);
    }
}

} // namespace ovtr::win32
