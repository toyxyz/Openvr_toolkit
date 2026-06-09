#include "platform/win32/WindowInput.h"

#include "platform/win32/AppState.h"
#include "platform/win32/Layout.h"
#include "platform/win32/MappingEditPanelLayout.h"
#include "platform/win32/MappingPanelLayout.h"
#include "platform/win32/ProfilePanelLayout.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {
namespace {

bool setHandCursor()
{
    SetCursor(LoadCursor(nullptr, IDC_HAND));
    return true;
}

} // namespace

bool handleMainWindowSetCursor(HWND hwnd, LPARAM lparam)
{
    if (LOWORD(lparam) != HTCLIENT) {
        return false;
    }

    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return false;
    }

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    POINT point{};
    GetCursorPos(&point);
    ScreenToClient(hwnd, &point);

    const RECT settingRect = topBarSettingRectForClient(clientWidth, clientHeight);
    if (PtInRect(&settingRect, point)) {
        return setHandCursor();
    }
    const RECT fileRect = topBarFileRectForClient(clientWidth, clientHeight);
    if (PtInRect(&fileRect, point)) {
        return setHandCursor();
    }

    const ViewportControlLayout viewportControls = viewportControlLayoutForClient(
        state,
        clientWidth,
        clientHeight
    );
    if (viewportControls.valid &&
        (PtInRect(&viewportControls.quadViewButtonRect, point) ||
         PtInRect(&viewportControls.showTextButtonRect, point) ||
         PtInRect(&viewportControls.showModelButtonRect, point) ||
         PtInRect(&viewportControls.smoothButtonRect, point) ||
         PtInRect(&viewportControls.lockButtonRect, point) ||
         PtInRect(&viewportControls.recordButtonRect, point))) {
        return setHandCursor();
    }
    if (viewportControls.animationValid &&
        (PtInRect(&viewportControls.firstFrameButtonRect, point) ||
         PtInRect(&viewportControls.playPauseButtonRect, point) ||
         PtInRect(&viewportControls.lastFrameButtonRect, point) ||
         PtInRect(&viewportControls.timelineRect, point) ||
         PtInRect(&viewportControls.closeButtonRect, point))) {
        return setHandCursor();
    }

    const OriginStepperButton originButton = originStepperButtonFromPoint(
        state,
        clientWidth,
        clientHeight,
        point
    );
    if (originButton.valid) {
        return setHandCursor();
    }

    const RECT deviceButtonRect = deviceToggleButtonRectForClient(state, clientWidth, clientHeight);
    if (PtInRect(&deviceButtonRect, point)) {
        return setHandCursor();
    }
    const RECT sessionButtonRect = sessionToggleButtonRectForClient(state, clientWidth, clientHeight);
    if (PtInRect(&sessionButtonRect, point)) {
        return setHandCursor();
    }
    const RECT streamingButtonRect = streamingToggleButtonRectForClient(state, clientWidth, clientHeight);
    if (PtInRect(&streamingButtonRect, point)) {
        return setHandCursor();
    }
    const StreamingPanelLayout streamingLayout = streamingPanelLayoutForClient(state, clientWidth, clientHeight);
    if (state->streamingPanelVisible && streamingLayout.valid &&
        (PtInRect(&streamingLayout.targetValueRect, point) ||
         PtInRect(&streamingLayout.hostValueRect, point) ||
         PtInRect(&streamingLayout.portValueRect, point) ||
         (state->streamingTargetDropdownOpen && PtInRect(&streamingLayout.targetDropdownRect, point)))) {
        return setHandCursor();
    }
    const RECT profileButtonRect = profileToggleButtonRectForClient(state, clientWidth, clientHeight);
    if (PtInRect(&profileButtonRect, point)) {
        return setHandCursor();
    }
    const RECT mappingButtonRect = mappingToggleButtonRectForClient(state, clientWidth, clientHeight);
    if (PtInRect(&mappingButtonRect, point)) {
        return setHandCursor();
    }
    const RECT editButtonRect = editToggleButtonRectForClient(state, clientWidth, clientHeight);
    if (PtInRect(&editButtonRect, point)) {
        return setHandCursor();
    }
    const ProfilePanelControlsLayout profileControls =
        profileControlsLayoutForPanel(profilePanelLayoutForClient(state, clientWidth, clientHeight));
    if (state->profilePanelVisible && profileControls.valid &&
        (PtInRect(&profileControls.previewButtonRect, point) ||
         PtInRect(&profileControls.saveButtonRect, point) ||
         PtInRect(&profileControls.loadButtonRect, point))) {
        return setHandCursor();
    }
    const MappingPanelRowLayout mappingRow = mappingRowLayoutAtPoint(
        profilePanelLayoutForClient(state, clientWidth, clientHeight),
        point,
        state->mappingScrollOffset
    );
    if (state->mappingPanelVisible && mappingRow.valid) {
        return setHandCursor();
    }
    const MappingPanelControlsLayout mappingControls =
        mappingControlsLayoutForPanel(profilePanelLayoutForClient(state, clientWidth, clientHeight));
    if (state->mappingPanelVisible && mappingControls.valid &&
        (PtInRect(&mappingControls.profileValueRect, point) ||
         PtInRect(&mappingControls.filterArmValueRect, point) ||
         PtInRect(&mappingControls.filterLegValueRect, point) ||
         PtInRect(&mappingControls.filterPinHandValueRect, point) ||
         PtInRect(&mappingControls.filterPinFootValueRect, point))) {
        return setHandCursor();
    }
    const ProfilePanelLayout rightPanel = profilePanelLayoutForClient(state, clientWidth, clientHeight);
    const MappingEditPanelRowLayout editRow = mappingEditRowAtPoint(
        rightPanel,
        state->mappingEditOffsetScrollOffset,
        point
    );
    const MappingEditAxisButton editButton = mappingEditAxisButtonAtPoint(rightPanel, point);
    const MappingEditPanelLayout editLayout = mappingEditPanelLayoutForPanel(rightPanel);
    const MappingEditStepOptionLayout editStepOption = mappingEditStepOptionAtPoint(rightPanel, point);
    const RECT editPresetDropdownRect = mappingEditOffsetPresetDropdownRectForPanel(rightPanel, 8);
    if (state->editPanelVisible &&
        (editRow.valid ||
         editButton.valid ||
         (editLayout.valid && PtInRect(&editLayout.listScrollbarRect, point)) ||
         (editLayout.valid && PtInRect(&editLayout.stepValueRect, point)) ||
         (editLayout.valid && PtInRect(&editLayout.presetNameEditRect, point)) ||
         (editLayout.valid && PtInRect(&editLayout.presetSaveButtonRect, point)) ||
         (editLayout.valid && PtInRect(&editLayout.presetValueRect, point)) ||
         (state->mappingEditOffsetPresetDropdownOpen && PtInRect(&editPresetDropdownRect, point)) ||
         (state->mappingEditStepDropdownOpen && editStepOption.valid))) {
        return setHandCursor();
    }
    const RECT profileSplitterRect = profileSplitterRectForClient(state, clientWidth, clientHeight);
    if ((state->profilePanelVisible || state->mappingPanelVisible || state->editPanelVisible) &&
        (state->profileSplitterDragging || PtInRect(&profileSplitterRect, point))) {
        SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
        return true;
    }

    const RECT debugResizeRect = debugResizeRectForClient(state, clientWidth, clientHeight);
    if (state->debugResizeDragging || PtInRect(&debugResizeRect, point)) {
        SetCursor(LoadCursor(nullptr, IDC_SIZENS));
        return true;
    }

    const RECT splitterRect = splitterRectForClient(state, clientWidth, clientHeight);
    if ((state->devicePanelVisible || state->sessionPanelVisible || state->streamingPanelVisible) &&
        (state->splitterDragging || PtInRect(&splitterRect, point))) {
        SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
        return true;
    }

    return false;
}

} // namespace ovtr::win32
