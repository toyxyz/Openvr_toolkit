#include "platform/win32/PaintWidgets.h"

#include "platform/win32/AppOriginState.h"
#include "platform/win32/Layout.h"
#include "platform/win32/OriginState.h"
#include "platform/win32/Win32GdiResources.h"

#include <array>
#include <string>
#include <vector>

namespace ovtr::win32 {
namespace {

void drawOriginStepperButton(HDC drawDc, const RECT& rect, const wchar_t* label)
{
    UniqueBrush buttonBrush(CreateSolidBrush(RGB(30, 34, 42)));
    UniquePen buttonPen(CreatePen(PS_SOLID, 1, RGB(67, 74, 88)));
    {
        SelectObjectGuard brushSelection(drawDc, buttonBrush.get());
        SelectObjectGuard penSelection(drawDc, buttonPen.get());
        RoundRect(drawDc, rect.left, rect.top, rect.right, rect.bottom, 4, 4);
    }

    RECT textRect = rect;
    SetTextColor(drawDc, RGB(225, 231, 240));
    DrawTextW(drawDc, label, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

} // namespace

void drawOriginStepperRow(
    HDC drawDc,
    HFONT labelFont,
    HFONT valueFont,
    const OriginPanelLayout& layout,
    const AppOriginState& state,
    const bool rotation
)
{
    const std::vector<OriginStepperAxisLayout> axes =
        originStepperAxisLayoutsForLayout(layout, rotation);
    if (axes.empty()) {
        return;
    }

    SelectObject(drawDc, labelFont);
    SetTextColor(drawDc, RGB(168, 180, 196));
    RECT rowLabelRect = originStepperRowLabelRect(layout, rotation);
    DrawTextW(
        drawDc,
        rotation ? L"Rot" : L"Pos",
        -1,
        &rowLabelRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );

    static constexpr const wchar_t* kAxisLabels[] = {L"X", L"Y", L"Z"};
    const std::array<float, 3>& values = rotation ? state.originRotationDegrees : state.originOffset;

    for (const OriginStepperAxisLayout& axisLayout : axes) {
        if (!axisLayout.valid || axisLayout.axis < 0 || axisLayout.axis >= 3) {
            continue;
        }

        SelectObject(drawDc, labelFont);
        SetTextColor(drawDc, RGB(168, 180, 196));
        RECT axisRect = axisLayout.axisLabelRect;
        DrawTextW(
            drawDc,
            kAxisLabels[axisLayout.axis],
            -1,
            &axisRect,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE
        );

        SelectObject(drawDc, valueFont);
        drawOriginStepperButton(drawDc, axisLayout.minusButton.rect, L"-");
        const std::wstring valueText =
            formatOriginStepperValue(values[static_cast<std::size_t>(axisLayout.axis)]);
        SetTextColor(drawDc, RGB(202, 211, 224));
        RECT valueRect = axisLayout.valueRect;
        DrawTextW(
            drawDc,
            valueText.c_str(),
            static_cast<int>(valueText.size()),
            &valueRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
        );
        drawOriginStepperButton(drawDc, axisLayout.plusButton.rect, L"+");
    }
}

} // namespace ovtr::win32
