#include "platform/win32/OriginState.h"

#include "platform/win32/AppOriginState.h"

#include <cmath>
#include <cwchar>
#include <iomanip>
#include <sstream>
#include <vector>

namespace ovtr::win32 {

std::wstring formatOriginPanelPosition(const AppOriginState& state)
{
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(3)
           << L"Position   X " << state.originOffset[0]
           << L"   Y " << state.originOffset[1]
           << L"   Z " << state.originOffset[2];
    return stream.str();
}

std::wstring formatOriginPanelRotation(const AppOriginState& state)
{
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(3)
           << L"Rotation   X " << state.originRotationDegrees[0]
           << L"   Y " << state.originRotationDegrees[1]
           << L"   Z " << state.originRotationDegrees[2];
    return stream.str();
}

std::wstring formatOriginEditorText(const AppOriginState& state)
{
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(6)
           << state.originOffset[0] << L" "
           << state.originOffset[1] << L" "
           << state.originOffset[2] << L" "
           << state.originRotationDegrees[0] << L" "
           << state.originRotationDegrees[1] << L" "
           << state.originRotationDegrees[2];
    return stream.str();
}

std::wstring formatOriginStepperValue(const float value)
{
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(3) << value;
    return stream.str();
}

bool parseOriginEditorText(
    const std::wstring& text,
    std::array<float, 3>& offset,
    std::array<float, 3>& rotation
)
{
    std::vector<float> values;
    values.reserve(6);

    const wchar_t* cursor = text.c_str();
    while (*cursor != L'\0' && values.size() < 6) {
        wchar_t* end = nullptr;
        const float value = std::wcstof(cursor, &end);
        if (end == cursor) {
            ++cursor;
            continue;
        }
        if (!std::isfinite(value)) {
            return false;
        }
        values.push_back(value);
        cursor = end;
    }

    if (values.size() < 6) {
        return false;
    }

    offset = {values[0], values[1], values[2]};
    rotation = {values[3], values[4], values[5]};
    return true;
}

bool originValuesAreZero(
    const std::array<float, 3>& offset,
    const std::array<float, 3>& rotation
) noexcept
{
    constexpr float kOriginZeroThreshold = 0.000001f;
    for (const float value : offset) {
        if (std::fabs(value) > kOriginZeroThreshold) {
            return false;
        }
    }
    for (const float value : rotation) {
        if (std::fabs(value) > kOriginZeroThreshold) {
            return false;
        }
    }
    return true;
}

} // namespace ovtr::win32
