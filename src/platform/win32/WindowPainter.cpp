#include "platform/win32/WindowPainter.h"

#include "platform/win32/WindowPaintFonts.h"
#include "platform/win32/WindowPaintScene.h"
#include "platform/win32/WindowStateAccess.h"
#include "platform/win32/Win32CompatibleDcResource.h"
#include "platform/win32/Win32GdiResources.h"
#include "platform/win32/Win32ScopedDcResources.h"

namespace ovtr::win32 {

void paintWindow(HWND hwnd)
{
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

    CompatibleDc bufferDcResource(paintDc);
    HDC bufferDc = bufferDcResource.get();
    UniqueBitmap bufferBitmap(
        bufferDc ? CreateCompatibleBitmap(paintDc, clientWidth, clientHeight) : nullptr
    );
    HGDIOBJ previousBitmap = nullptr;
    HDC drawDc = paintDc;
    if (bufferDc && bufferBitmap) {
        previousBitmap = SelectObject(bufferDc, bufferBitmap.get());
        drawDc = bufferDc;
    }

    UniqueBrush background(CreateSolidBrush(RGB(24, 26, 30)));
    FillRect(drawDc, &clientRect, background.get());

    SetBkMode(drawDc, TRANSPARENT);
    SetTextColor(drawDc, RGB(230, 234, 240));

    const WindowPaintFonts fonts = createWindowPaintFonts();

    AppWindowState* state = appStateForWindow(hwnd);
    paintWindowScene(drawDc, state, fonts, clientWidth, clientHeight);

    if (drawDc == bufferDc) {
        BitBlt(paintDc, 0, 0, clientWidth, clientHeight, bufferDc, 0, 0, SRCCOPY);
    }

    if (previousBitmap) {
        SelectObject(bufferDc, previousBitmap);
    }
}

} // namespace ovtr::win32
