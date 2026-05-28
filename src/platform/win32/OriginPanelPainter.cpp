#include "platform/win32/OriginPanelPainter.h"

#include "platform/win32/AppOriginState.h"
#include "platform/win32/PaintWidgets.h"
#include "platform/win32/Win32GdiResources.h"

#include <string>

namespace ovtr::win32 {
namespace {

constexpr int kOriginPanelPadding = 12;

} // namespace

void paintOriginPanel(
    HDC drawDc,
    HFONT labelFont,
    HFONT valueFont,
    const OriginPanelLayout& layout,
    const AppOriginState& state
)
{
    if (!layout.valid) {
        return;
    }

    UniqueBrush panelBrush(CreateSolidBrush(RGB(20, 23, 28)));
    FillRect(drawDc, &layout.boxRect, panelBrush.get());

    UniquePen panelPen(CreatePen(PS_SOLID, 1, RGB(58, 64, 76)));
    {
        SelectObjectGuard penSelection(drawDc, panelPen.get());
        SelectObjectGuard brushSelection(drawDc, GetStockObject(NULL_BRUSH));
        Rectangle(
            drawDc,
            layout.boxRect.left,
            layout.boxRect.top,
            layout.boxRect.right,
            layout.boxRect.bottom
        );
    }

    SelectObject(drawDc, labelFont);
    SetTextColor(drawDc, RGB(168, 180, 196));
    RECT originTitleRect{
        layout.boxRect.left + kOriginPanelPadding,
        layout.boxRect.top + 8,
        layout.boxRect.right - kOriginPanelPadding,
        layout.boxRect.top + 32
    };
    DrawTextW(drawDc, L"Origin", -1, &originTitleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    RECT originStateRect = originTitleRect;
    const std::wstring originStateText = state.originEnabled ? L"Enabled" : L"Disabled";
    SetTextColor(drawDc, state.originEnabled ? RGB(180, 216, 174) : RGB(156, 166, 178));
    DrawTextW(
        drawDc,
        originStateText.c_str(),
        static_cast<int>(originStateText.size()),
        &originStateRect,
        DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );

    const bool editingOrigin = state.originEditWindow != nullptr && IsWindow(state.originEditWindow);
    if (editingOrigin) {
        return;
    }

    drawOriginStepperRow(drawDc, labelFont, valueFont, layout, state, false);
    drawOriginStepperRow(drawDc, labelFont, valueFont, layout, state, true);
}

} // namespace ovtr::win32
