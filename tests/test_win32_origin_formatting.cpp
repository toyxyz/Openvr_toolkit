#include "TestCases.h"
#include "TestSupport.h"

#include "platform/win32/AppOriginState.h"
#include "platform/win32/OriginState.h"

#include <array>

namespace ovtr::test {

void testWin32OriginFormatting()
{
    ovtr::win32::AppOriginState state;
    state.originOffset = {1.25f, -2.0f, 0.5f};
    state.originRotationDegrees = {10.0f, 20.5f, -30.25f};

    require(
        ovtr::win32::formatOriginPanelPosition(state) == L"Position   X 1.250   Y -2.000   Z 0.500",
        "origin position panel text"
    );
    require(
        ovtr::win32::formatOriginPanelRotation(state) == L"Rotation   X 10.000   Y 20.500   Z -30.250",
        "origin rotation panel text"
    );
    require(
        ovtr::win32::formatOriginEditorText(state) ==
            L"1.250000 -2.000000 0.500000 10.000000 20.500000 -30.250000",
        "origin editor text"
    );
    require(ovtr::win32::formatOriginStepperValue(-1.5f) == L"-1.500", "origin stepper value text");

    std::array<float, 3> offset{};
    std::array<float, 3> rotation{};
    require(
        ovtr::win32::parseOriginEditorText(L"x=1 y=-2 z=3 rx=4 ry=5 rz=6 extra=7", offset, rotation),
        "origin editor parses first six numeric values"
    );
    require(offset == std::array<float, 3>{1.0f, -2.0f, 3.0f}, "origin editor parsed offset");
    require(rotation == std::array<float, 3>{4.0f, 5.0f, 6.0f}, "origin editor parsed rotation");
    require(!ovtr::win32::parseOriginEditorText(L"1 2 3 4 5", offset, rotation), "origin editor rejects missing values");
    require(!ovtr::win32::parseOriginEditorText(L"1 2 3 4 5 1e999", offset, rotation), "origin editor rejects non-finite values");

    require(
        ovtr::win32::originValuesAreZero({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}),
        "origin zero check accepts zeros"
    );
    require(
        ovtr::win32::originValuesAreZero({0.0000004f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}),
        "origin zero check accepts tiny values"
    );
    require(
        !ovtr::win32::originValuesAreZero({0.00001f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}),
        "origin zero check rejects meaningful offset"
    );
}

} // namespace ovtr::test
