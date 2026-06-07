#include "platform/win32/StreamingPanelPainter.h"

#include "platform/win32/AppState.h"
#include "platform/win32/StreamingOutputTarget.h"
#include "platform/win32/Win32GdiResources.h"
#include "platform/win32/Win32String.h"

#include <iomanip>
#include <sstream>
#include <string>

namespace ovtr::win32 {
namespace {

void drawBox(HDC drawDc, const RECT& rect)
{
    UniqueBrush brush(CreateSolidBrush(RGB(20, 23, 28)));
    FillRect(drawDc, &rect, brush.get());
    UniquePen pen(CreatePen(PS_SOLID, 1, RGB(58, 64, 76)));
    SelectObjectGuard penSelection(drawDc, pen.get());
    SelectObjectGuard brushSelection(drawDc, GetStockObject(NULL_BRUSH));
    Rectangle(drawDc, rect.left, rect.top, rect.right, rect.bottom);
}

void drawRow(
    HDC drawDc,
    HFONT font,
    const RECT& boxRect,
    const RECT& labelRect,
    const RECT& valueRect,
    const wchar_t* label,
    const std::wstring& value,
    const bool dropdown
) {
    UniqueBrush brush(CreateSolidBrush(RGB(18, 22, 28)));
    FillRect(drawDc, &boxRect, brush.get());
    UniquePen pen(CreatePen(PS_SOLID, 1, RGB(67, 74, 88)));
    SelectObjectGuard penSelection(drawDc, pen.get());
    SelectObjectGuard brushSelection(drawDc, GetStockObject(NULL_BRUSH));
    Rectangle(drawDc, boxRect.left, boxRect.top, boxRect.right, boxRect.bottom);
    MoveToEx(drawDc, labelRect.right + 8, boxRect.top, nullptr);
    LineTo(drawDc, labelRect.right + 8, boxRect.bottom);

    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, RGB(168, 180, 196));
    RECT drawLabel = labelRect;
    DrawTextW(drawDc, label, -1, &drawLabel, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SetTextColor(drawDc, RGB(225, 231, 240));
    RECT drawValue = valueRect;
    drawValue.left += 6;
    drawValue.right -= dropdown ? 18 : 6;
    DrawTextW(drawDc, value.c_str(), -1, &drawValue, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (dropdown) {
        RECT glyph{valueRect.right - 18, valueRect.top, valueRect.right - 2, valueRect.bottom};
        DrawTextW(drawDc, L"v", -1, &glyph, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

void drawDropdown(HDC drawDc, HFONT font, const AppWindowState& state, const RECT& rect)
{
    drawBox(drawDc, rect);
    SelectObjectGuard fontSelection(drawDc, font);
    const wchar_t* labels[2] = {L"None", L"VMC"};
    const int rowHeight = (rect.bottom - rect.top) / 2;
    for (int row = 0; row < 2; ++row) {
        RECT rowRect{rect.left + 1, rect.top + row * rowHeight, rect.right - 1, rect.top + (row + 1) * rowHeight};
        const bool selected = (row == 0 && state.streamingOutputTarget == StreamingOutputTarget::None) ||
            (row == 1 && state.streamingOutputTarget == StreamingOutputTarget::Vmc);
        if (selected) {
            UniqueBrush selectedBrush(CreateSolidBrush(RGB(48, 78, 118)));
            FillRect(drawDc, &rowRect, selectedBrush.get());
        }
        SetTextColor(drawDc, selected ? RGB(236, 242, 250) : RGB(202, 211, 224));
        rowRect.left += 8;
        DrawTextW(drawDc, labels[row], -1, &rowRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
}

std::wstring angleText(const float degrees)
{
    std::wostringstream out;
    out << std::fixed << std::setprecision(3) << degrees;
    std::wstring text = out.str();
    while (text.size() > 1 && text.back() == L'0') {
        text.pop_back();
    }
    if (!text.empty() && text.back() == L'.') {
        text.pop_back();
    }
    return text;
}

} // namespace

void paintStreamingPanel(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    const AppWindowState& state,
    const StreamingPanelLayout& layout
) {
    if (!layout.valid) {
        return;
    }
    SetBkMode(drawDc, TRANSPARENT);
    drawBox(drawDc, layout.boxRect);

    SelectObjectGuard headerSelection(drawDc, headerFont ? headerFont : bodyFont);
    SetTextColor(drawDc, RGB(168, 180, 196));
    RECT headerText = layout.headerRect;
    DrawTextW(drawDc, L"Streaming", -1, &headerText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    drawRow(
        drawDc,
        bodyFont,
        layout.targetBoxRect,
        layout.targetLabelRect,
        layout.targetValueRect,
        L"Target",
        streamingOutputTargetLabel(state.streamingOutputTarget),
        true
    );

    if (layout.vmcVisible) {
        drawBox(drawDc, layout.vmcBoxRect);
        SetTextColor(drawDc, RGB(168, 180, 196));
        RECT vmcHeader = layout.vmcHeaderRect;
        DrawTextW(drawDc, L"VMC", -1, &vmcHeader, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        drawRow(drawDc, bodyFont, layout.hostBoxRect, layout.hostLabelRect,
            layout.hostValueRect, L"Host", widen(state.vmcSendHost), false);
        drawRow(drawDc, bodyFont, layout.portBoxRect, layout.portLabelRect,
            layout.portValueRect, L"Port", std::to_wstring(state.vmcSendPort), false);
        drawRow(drawDc, bodyFont, layout.armSpacingBoxRect, layout.armSpacingLabelRect,
            layout.armSpacingValueRect, L"Arm spacing", angleText(state.vmcArmSpacingDegrees), false);
        drawRow(drawDc, bodyFont, layout.legSpacingBoxRect, layout.legSpacingLabelRect,
            layout.legSpacingValueRect, L"Leg spacing", angleText(state.vmcLegSpacingDegrees), false);
    }
    if (state.streamingTargetDropdownOpen) {
        drawDropdown(drawDc, bodyFont, state, layout.targetDropdownRect);
    }
}

} // namespace ovtr::win32
