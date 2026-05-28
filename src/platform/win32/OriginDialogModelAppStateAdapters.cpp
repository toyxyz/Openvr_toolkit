#include "platform/win32/OriginDialogModel.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

OriginDialogValues originDialogValuesFromState(const AppWindowState& state)
{
    return originDialogValuesFromState(static_cast<const AppOriginState&>(state));
}

void applyOriginDialogValuesToState(AppWindowState& state, const OriginDialogValues& values)
{
    applyOriginDialogValuesToState(static_cast<AppOriginState&>(state), values);
}

} // namespace ovtr::win32
