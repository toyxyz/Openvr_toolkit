#include "platform/win32/StatusBarPainter.h"
#include "platform/win32/UiTheme.h"

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
    UniqueBrush statusBarBrush(CreateSolidBrush(ui::toolbar));
    FillRect(drawDc, &statusBarRect, statusBarBrush.get());

    UniquePen statusBorderPen(CreatePen(PS_SOLID, 1, ui::border));
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

    SetTextColor(drawDc, ui::text);
    DrawTextW(
        drawDc,
        statusMessage.c_str(),
        static_cast<int>(statusMessage.size()),
        &messageRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );
    SetTextColor(drawDc, ui::muted);
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
        state && state->debugMonitorVisible ? ui::accent : ui::button
    ));
    UniquePen buttonPen(CreatePen(
        PS_SOLID,
        1,
        state && state->debugMonitorVisible ? ui::accent : ui::buttonBorder
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
    SetTextColor(drawDc, ui::text);
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
