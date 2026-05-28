#include "platform/win32/OriginDialogModel.h"

#include "platform/win32/AppOriginState.h"
#include "platform/win32/OriginState.h"

#include <mutex>

namespace ovtr::win32 {

OriginDialogValues originDialogValuesFromState(const AppOriginState& state)
{
    OriginDialogValues values;
    values.enabled = state.originEnabled;
    values.offset = state.originOffset;
    values.rotationDegrees = state.originRotationDegrees;
    return values;
}

bool originDialogValuesActive(const OriginDialogValues& values) noexcept
{
    return values.enabled && !originValuesAreZero(values.offset, values.rotationDegrees);
}

void applyOriginDialogValuesToState(AppOriginState& state, const OriginDialogValues& values)
{
    const bool active = originDialogValuesActive(values);
    std::lock_guard<std::mutex> lock(state.originMutex);
    state.originEnabled = active;
    state.originOffset = values.offset;
    state.originRotationDegrees = values.rotationDegrees;
    state.selectedOriginRuntimeIndex = kNoSelectedRuntimeIndex;
}

} // namespace ovtr::win32
