#include "platform/win32/OriginState.h"

#include "platform/win32/AppOriginState.h"
#include "platform/win32/AppStateConstants.h"

#include <cmath>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace ovtr::win32 {

bool adjustOriginAxis(
    AppOriginState& state,
    const bool rotation,
    const int axis,
    const float delta
)
{
    if (axis < 0 || axis >= 3) {
        return false;
    }

    std::lock_guard<std::mutex> lock(state.originMutex);
    std::array<float, 3>& values = rotation
        ? state.originRotationDegrees
        : state.originOffset;
    values[static_cast<std::size_t>(axis)] += delta;
    if (std::fabs(values[static_cast<std::size_t>(axis)]) < 0.0000005f) {
        values[static_cast<std::size_t>(axis)] = 0.0f;
    }

    state.selectedOriginRuntimeIndex = kNoSelectedRuntimeIndex;
    state.originEnabled = !originValuesAreZero(state.originOffset, state.originRotationDegrees);

    static constexpr const char* kAxisNames[] = {"X", "Y", "Z"};
    std::ostringstream stream;
    stream << "origin "
           << (rotation ? "rotation " : "position ")
           << kAxisNames[axis]
           << " adjusted to "
           << std::fixed << std::setprecision(3)
           << values[static_cast<std::size_t>(axis)];
    state.originStatusMessage = stream.str();
    return true;
}

} // namespace ovtr::win32
