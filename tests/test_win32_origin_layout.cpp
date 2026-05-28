#include "TestCases.h"
#include "TestSupport.h"
#include "Win32LayoutTestSupport.h"

#include "platform/win32/Layout.h"

#include <vector>

namespace ovtr::test {

void testWin32OriginLayout()
{
    const ovtr::win32::OriginPanelLayout originLayout{
        RECT{40, 100, 420, 228},
        RECT{52, 130, 408, 202},
        true
    };
    require(
        sameRect(ovtr::win32::originEditorRectForLayout(originLayout), 52, 130, 408, 156),
        "origin editor rect"
    );
    require(
        sameRect(ovtr::win32::originStepperRowRect(originLayout, false), 52, 130, 408, 160),
        "origin position stepper row"
    );
    require(
        sameRect(ovtr::win32::originStepperRowRect(originLayout, true), 52, 160, 408, 190),
        "origin rotation stepper row"
    );
    require(
        sameRect(ovtr::win32::originStepperRowLabelRect(originLayout, false), 52, 130, 86, 160),
        "origin position row label rect"
    );
    require(
        sameRect(ovtr::win32::originStepperRowLabelRect(originLayout, true), 52, 160, 86, 190),
        "origin rotation row label rect"
    );

    const std::vector<ovtr::win32::OriginStepperAxisLayout> positionAxes =
        ovtr::win32::originStepperAxisLayoutsForLayout(originLayout, false);
    require(positionAxes.size() == 3, "origin position axis layout count");
    require(positionAxes[0].valid, "origin x axis layout valid");
    require(sameRect(positionAxes[0].axisLabelRect, 86, 130, 98, 160), "origin x axis label rect");
    require(sameRect(positionAxes[0].valueRect, 120, 130, 169, 160), "origin x value rect");
    require(sameRect(positionAxes[0].minusButton.rect, 100, 136, 118, 154), "origin x layout minus rect");
    require(sameRect(positionAxes[0].plusButton.rect, 171, 136, 189, 154), "origin x layout plus rect");
    require(!positionAxes[0].rotation, "origin x axis layout is position");
    require(positionAxes[0].axis == 0, "origin x axis layout index");
    require(nearFloat(positionAxes[0].minusButton.delta, -0.001f), "origin x layout minus delta");
    require(nearFloat(positionAxes[0].plusButton.delta, 0.001f), "origin x layout plus delta");

    const std::vector<ovtr::win32::OriginStepperAxisLayout> rotationAxes =
        ovtr::win32::originStepperAxisLayoutsForLayout(originLayout, true);
    require(rotationAxes.size() == 3, "origin rotation axis layout count");
    require(rotationAxes[0].rotation, "origin rx axis layout is rotation");
    require(sameRect(rotationAxes[0].axisLabelRect, 86, 160, 98, 190), "origin rx axis label rect");
    require(sameRect(rotationAxes[0].minusButton.rect, 100, 166, 118, 184), "origin rx layout minus rect");
    require(nearFloat(rotationAxes[0].minusButton.delta, -0.1f), "origin rx layout minus delta");

    const std::vector<ovtr::win32::OriginStepperButton> originButtons =
        ovtr::win32::originStepperButtonsForLayout(originLayout);
    require(originButtons.size() == 12, "origin stepper button count");
    require(sameRect(originButtons[0].rect, 100, 136, 118, 154), "origin x minus rect");
    require(!originButtons[0].rotation, "origin x minus is position button");
    require(originButtons[0].axis == 0, "origin x minus axis");
    require(nearFloat(originButtons[0].delta, -0.001f), "origin x minus delta");
    require(sameRect(originButtons[1].rect, 171, 136, 189, 154), "origin x plus rect");
    require(nearFloat(originButtons[1].delta, 0.001f), "origin x plus delta");
    require(sameRect(originButtons[6].rect, 100, 166, 118, 184), "origin rx minus rect");
    require(originButtons[6].rotation, "origin rx minus is rotation button");
    require(nearFloat(originButtons[6].delta, -0.1f), "origin rx minus delta");

    const ovtr::win32::OriginStepperButton hitOriginButton =
        ovtr::win32::originStepperButtonFromPoint(originLayout, POINT{172, 140});
    require(hitOriginButton.valid, "origin stepper hit test finds button");
    require(hitOriginButton.axis == 0, "origin stepper hit test axis");
    require(nearFloat(hitOriginButton.delta, 0.001f), "origin stepper hit test delta");
    require(
        !ovtr::win32::originStepperButtonFromPoint(originLayout, POINT{52, 130}).valid,
        "origin stepper hit test rejects label"
    );
    require(
        !ovtr::win32::originStepperButtonFromPoint(ovtr::win32::OriginPanelLayout{}, POINT{172, 140}).valid,
        "origin stepper rejects invalid layout"
    );
}

} // namespace ovtr::test
