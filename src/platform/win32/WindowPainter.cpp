#include "platform/win32/WindowPainter.h"
#include "platform/win32/PerformanceCapture.h"
#include "platform/win32/UiTheme.h"

#include "platform/win32/WindowPaintFonts.h"
#include "platform/win32/WindowPaintScene.h"
#include "platform/win32/WindowStateAccess.h"
#include "platform/win32/Win32CompatibleDcResource.h"
#include "platform/win32/Win32GdiResources.h"
#include "platform/win32/Win32ScopedDcResources.h"

namespace ovtr::win32 {

void paintWindow(HWND hwnd)
{
    PerformanceScope trace("paint");
    PaintDc paint(hwnd);
    HDC paintDc = paint.get();
    if (!paintDc) {
        return;
    }

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    if (clientWidth <= 0 || clientHeight <= 0) {
        return;
    }
    RECT dirtyRect = paint.paint().rcPaint;
    if (IsRectEmpty(&dirtyRect)) dirtyRect = clientRect;
    const int dirtyWidth = dirtyRect.right - dirtyRect.left;
    const int dirtyHeight = dirtyRect.bottom - dirtyRect.top;

    CompatibleDc bufferDcResource(paintDc);
    HDC bufferDc = bufferDcResource.get();
    UniqueBitmap bufferBitmap(
        bufferDc ? CreateCompatibleBitmap(paintDc, dirtyWidth, dirtyHeight) : nullptr
    );
    HGDIOBJ previousBitmap = nullptr;
    HDC drawDc = paintDc;
    if (bufferDc && bufferBitmap) {
        previousBitmap = SelectObject(bufferDc, bufferBitmap.get());
        SetViewportOrgEx(bufferDc, -dirtyRect.left, -dirtyRect.top, nullptr);
        drawDc = bufferDc;
    }

    UniqueBrush background(CreateSolidBrush(ui::window));
    FillRect(drawDc, &clientRect, background.get());

    SetBkMode(drawDc, TRANSPARENT);
    SetTextColor(drawDc, ui::text);

    static const WindowPaintFonts fonts = createWindowPaintFonts();

    AppWindowState* state = appStateForWindow(hwnd);
    paintWindowScene(drawDc, state, fonts, clientWidth, clientHeight);

    if (drawDc == bufferDc) {
        BitBlt(
            paintDc,
            dirtyRect.left,
            dirtyRect.top,
            dirtyWidth,
            dirtyHeight,
            bufferDc,
            dirtyRect.left,
            dirtyRect.top,
            SRCCOPY
        );
    }

    if (previousBitmap) {
        SelectObject(bufferDc, previousBitmap);
    }
}

} // namespace ovtr::win32
