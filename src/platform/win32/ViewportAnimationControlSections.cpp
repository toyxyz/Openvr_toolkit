#include "platform/win32/ViewportAnimationControlSections.h"
#include "platform/win32/UiTheme.h"

namespace ovtr::win32 {
namespace {

enum class TransportIcon { First, Play, Pause, Last };

void drawTransport(HDC dc, const RECT& rect, const TransportIcon icon, const bool enabled, const bool active)
{
    ui::buttonSurface(dc, rect, active);
    const int x = (rect.left + rect.right) / 2;
    const int y = (rect.top + rect.bottom) / 2;
    const COLORREF color = enabled ? ui::text : ui::disabled;
    UniqueBrush brush(CreateSolidBrush(color));
    SelectObjectGuard brushSelection(dc, brush.get());
    SelectObjectGuard penSelection(dc, GetStockObject(NULL_PEN));
    if (icon == TransportIcon::Pause) {
        Rectangle(dc, x - 5, y - 5, x - 2, y + 5);
        Rectangle(dc, x + 2, y - 5, x + 5, y + 5);
    } else {
        const bool first = icon == TransportIcon::First;
        POINT points[]{{x + (first ? 4 : -4), y - 5},
            {x + (first ? 4 : -4), y + 5}, {x + (first ? -3 : 4), y}};
        Polygon(dc, points, 3);
        if (icon == TransportIcon::First) Rectangle(dc, x - 6, y - 5, x - 4, y + 5);
        if (icon == TransportIcon::Last) Rectangle(dc, x + 5, y - 5, x + 7, y + 5);
    }
}

} // namespace

void drawImportedAnimationButtons(HDC dc, HFONT font, const ViewportControlLayout& layout,
    const bool playing, const bool enabled, const bool showClose)
{
    drawTransport(dc, layout.firstFrameButtonRect, TransportIcon::First, enabled, false);
    drawTransport(dc, layout.playPauseButtonRect, playing ? TransportIcon::Pause : TransportIcon::Play, enabled, playing);
    drawTransport(dc, layout.lastFrameButtonRect, TransportIcon::Last, enabled, false);
    if (enabled && showClose) {
        ui::buttonSurface(dc, layout.closeButtonRect);
        SelectObjectGuard fontSelection(dc, font);
        SetTextColor(dc, ui::text);
        SetBkMode(dc, TRANSPARENT);
        RECT text = layout.closeButtonRect;
        DrawTextW(dc, L"Close", -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
}

} // namespace ovtr::win32
