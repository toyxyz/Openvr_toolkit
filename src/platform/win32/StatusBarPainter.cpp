#include "platform/win32/StatusBarPainter.h"

#include "platform/win32/Layout.h"
#include "platform/win32/Win32GdiResources.h"

#include <string>

namespace ovtr::win32 {
namespace {

constexpr int kContentMargin = 32;

} // namespace

void paintStatusBar(
    HDC drawDc,
    HFONT font,
    const StatusBarPaintState* state,
    const int clientWidth,
    const int clientHeight,
    const int statusBarTop
)
{
    RECT statusBarRect{0, statusBarTop, clientWidth, clientHeight};
    UniqueBrush statusBarBrush(CreateSolidBrush(RGB(18, 20, 24)));
    FillRect(drawDc, &statusBarRect, statusBarBrush.get());

    UniquePen statusBorderPen(CreatePen(PS_SOLID, 1, RGB(54, 58, 66)));
    {
        SelectObjectGuard penSelection(drawDc, statusBorderPen.get());
        MoveToEx(drawDc, 0, statusBarTop, nullptr);
        LineTo(drawDc, clientWidth, statusBarTop);
    }

    SelectObject(drawDc, font);
    const std::wstring statusMessage = state
        ? L"Status: " + state->statusMessage
        : L"Status: Loading...";
    const std::wstring statusMetrics = state ? state->statusMetrics : std::wstring{};
    const int statusSplitX = clientWidth > 760 ? clientWidth / 2 : (clientWidth * 45) / 100;
    const RECT debugButtonRect = debugButtonRectForClient(clientWidth, clientHeight);
    RECT messageRect{kContentMargin, statusBarTop, statusSplitX - 12, clientHeight};
    RECT metricsRect{statusSplitX, statusBarTop, debugButtonRect.left - 12, clientHeight};

    SetTextColor(drawDc, RGB(226, 230, 236));
    DrawTextW(
        drawDc,
        statusMessage.c_str(),
        static_cast<int>(statusMessage.size()),
        &messageRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );
    SetTextColor(drawDc, RGB(166, 176, 190));
    if (metricsRect.right > metricsRect.left) {
        DrawTextW(
            drawDc,
            statusMetrics.c_str(),
            static_cast<int>(statusMetrics.size()),
            &metricsRect,
            DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
        );
    }

    UniqueBrush buttonBrush(CreateSolidBrush(
        state && state->debugMonitorVisible ? RGB(48, 63, 82) : RGB(30, 34, 42)
    ));
    UniquePen buttonPen(CreatePen(
        PS_SOLID,
        1,
        state && state->debugMonitorVisible ? RGB(92, 126, 168) : RGB(67, 74, 88)
    ));
    {
        SelectObjectGuard brushSelection(drawDc, buttonBrush.get());
        SelectObjectGuard penSelection(drawDc, buttonPen.get());
        RoundRect(
            drawDc,
            debugButtonRect.left,
            debugButtonRect.top,
            debugButtonRect.right,
            debugButtonRect.bottom,
            6,
            6
        );
    }

    SelectObject(drawDc, font);
    SetTextColor(drawDc, RGB(225, 231, 240));
    RECT debugButtonTextRect = debugButtonRect;
    DrawTextW(
        drawDc,
        L"Debug",
        -1,
        &debugButtonTextRect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );
}

} // namespace ovtr::win32
