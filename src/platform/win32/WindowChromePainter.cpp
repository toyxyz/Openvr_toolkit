#include "platform/win32/WindowChromePainter.h"

#include "platform/win32/AppTopBarState.h"
#include "platform/win32/PaintWidgets.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {
namespace {

constexpr int kTopBarHeight = 32;

} // namespace

void paintTopBar(
    HDC drawDc,
    HFONT font,
    const AppTopBarState* state,
    const int clientWidth,
    const int clientHeight
)
{
    const RECT topBarRect{0, 0, clientWidth, clientHeight < kTopBarHeight ? clientHeight : kTopBarHeight};
    if (topBarRect.bottom <= topBarRect.top) {
        return;
    }

    UniqueBrush topBarBrush(CreateSolidBrush(RGB(18, 20, 24)));
    FillRect(drawDc, &topBarRect, topBarBrush.get());

    UniquePen topBarPen(CreatePen(PS_SOLID, 1, RGB(54, 58, 66)));
    {
        SelectObjectGuard penSelection(drawDc, topBarPen.get());
        MoveToEx(drawDc, 0, topBarRect.bottom - 1, nullptr);
        LineTo(drawDc, clientWidth, topBarRect.bottom - 1);
    }

    drawTopBarMenuButton(
        drawDc,
        font,
        topBarFileRectForClient(clientWidth, clientHeight),
        L"File",
        state && state->activeTopBarMenu == ActiveTopBarMenu::File
    );
    drawTopBarMenuButton(
        drawDc,
        font,
        topBarSettingRectForClient(clientWidth, clientHeight),
        L"Setting",
        state && state->activeTopBarMenu == ActiveTopBarMenu::Setting
    );
}

} // namespace ovtr::win32
