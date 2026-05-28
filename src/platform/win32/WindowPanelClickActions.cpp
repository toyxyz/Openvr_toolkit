#include "platform/win32/WindowClickActionSections.h"

#include "platform/win32/AppState.h"
#include "platform/win32/OriginEditor.h"
#include "platform/win32/WindowLayout.h"

namespace ovtr::win32 {

bool handleOriginStepperClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const OriginStepperButton originButton = originStepperButtonFromPoint(
        &state,
        clientWidth,
        clientHeight,
        point
    );
    if (!originButton.valid) {
        return false;
    }

    applyOriginStepperButton(hwnd, state, originButton);
    return true;
}

} // namespace ovtr::win32
