#include "platform/win32/WindowLayout.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

OriginPanelLayout originPanelLayoutForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    (void)state;
    (void)clientWidth;
    (void)clientHeight;
    return {};
}

RECT originEditorRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    return originEditorRectForLayout(originPanelLayoutForClient(state, clientWidth, clientHeight));
}

OriginStepperButton originStepperButtonFromPoint(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    return originStepperButtonFromPoint(
        originPanelLayoutForClient(state, clientWidth, clientHeight),
        point
    );
}

} // namespace ovtr::win32
