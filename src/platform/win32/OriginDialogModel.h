#pragma once

#include <array>

namespace ovtr::win32 {

struct AppOriginState;
struct AppWindowState;

struct OriginDialogValues {
    bool enabled = false;
    std::array<float, 3> offset{0.0f, 0.0f, 0.0f};
    std::array<float, 3> rotationDegrees{0.0f, 0.0f, 0.0f};
};

OriginDialogValues originDialogValuesFromState(const AppOriginState& state);
OriginDialogValues originDialogValuesFromState(const AppWindowState& state);
bool originDialogValuesActive(const OriginDialogValues& values) noexcept;
void applyOriginDialogValuesToState(AppOriginState& state, const OriginDialogValues& values);
void applyOriginDialogValuesToState(AppWindowState& state, const OriginDialogValues& values);

} // namespace ovtr::win32
