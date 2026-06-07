#include "platform/win32/WindowClickActionSections.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/OriginEditor.h"
#include "platform/win32/SessionEditor.h"
#include "platform/win32/StreamingOutputTarget.h"
#include "platform/win32/StreamingPanelEditor.h"
#include "platform/win32/VmcStreamingPose.h"
#include "platform/win32/WindowLayout.h"

#include <string>

namespace ovtr::win32 {

bool handleStreamingToggleClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const RECT buttonRect = streamingToggleButtonRectForClient(&state, clientWidth, clientHeight);
    if (!PtInRect(&buttonRect, point)) {
        return false;
    }

    state.streamingPanelVisible = !state.streamingPanelVisible;
    if (state.streamingPanelVisible) {
        state.devicePanelVisible = false;
        state.sessionPanelVisible = false;
        if (state.originEditWindow) {
            closeOriginEditor(hwnd, state);
        }
        if (state.sessionEditWindow) {
            closeSessionEditor(hwnd, state);
        }
    } else {
        state.streamingTargetDropdownOpen = false;
        closeStreamingHostEditor(hwnd, state);
        closeStreamingPortEditor(hwnd, state);
        closeStreamingSpacingEditors(hwnd, state);
    }
    state.splitterDragging = false;
    appendDebugLog(
        state,
        state.streamingPanelVisible ? L"Streaming panel opened" : L"Streaming panel closed"
    );
    layoutChildWindows(hwnd);
    invalidateWindowLayout(hwnd);
    return true;
}

bool handleStreamingPanelClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const StreamingPanelLayout layout = streamingPanelLayoutForClient(&state, clientWidth, clientHeight);
    if (!layout.valid) {
        return false;
    }
    if (state.streamingTargetDropdownOpen && PtInRect(&layout.targetDropdownRect, point)) {
        const int rowHeight = (layout.targetDropdownRect.bottom - layout.targetDropdownRect.top) / 2;
        const int row = rowHeight > 0 ? (point.y - layout.targetDropdownRect.top) / rowHeight : 0;
        state.streamingOutputTarget = row == 0 ? StreamingOutputTarget::None : StreamingOutputTarget::Vmc;
        state.streamingTargetDropdownOpen = false;
        if (state.streamingOutputTarget == StreamingOutputTarget::None) {
            stopVmcStreamingOutput(state);
        }
        closeStreamingHostEditor(hwnd, state);
        closeStreamingPortEditor(hwnd, state);
        closeStreamingSpacingEditors(hwnd, state);
        appendDebugLog(state, std::wstring(L"Streaming target: ") +
            streamingOutputTargetLabel(state.streamingOutputTarget));
        layoutChildWindows(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }
    if (PtInRect(&layout.targetValueRect, point)) {
        state.streamingTargetDropdownOpen = !state.streamingTargetDropdownOpen;
        closeStreamingHostEditor(hwnd, state);
        closeStreamingPortEditor(hwnd, state);
        closeStreamingSpacingEditors(hwnd, state);
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }
    if (layout.vmcVisible && PtInRect(&layout.hostValueRect, point)) {
        state.streamingTargetDropdownOpen = false;
        closeStreamingPortEditor(hwnd, state);
        closeStreamingSpacingEditors(hwnd, state);
        showStreamingHostEditor(hwnd, state);
        return true;
    }
    if (layout.vmcVisible && PtInRect(&layout.portValueRect, point)) {
        state.streamingTargetDropdownOpen = false;
        closeStreamingHostEditor(hwnd, state);
        closeStreamingSpacingEditors(hwnd, state);
        showStreamingPortEditor(hwnd, state);
        return true;
    }
    if (layout.vmcVisible && PtInRect(&layout.armSpacingValueRect, point)) {
        state.streamingTargetDropdownOpen = false;
        closeStreamingHostEditor(hwnd, state);
        closeStreamingPortEditor(hwnd, state);
        closeStreamingLegSpacingEditor(hwnd, state);
        showStreamingArmSpacingEditor(hwnd, state);
        return true;
    }
    if (layout.vmcVisible && PtInRect(&layout.legSpacingValueRect, point)) {
        state.streamingTargetDropdownOpen = false;
        closeStreamingHostEditor(hwnd, state);
        closeStreamingPortEditor(hwnd, state);
        closeStreamingArmSpacingEditor(hwnd, state);
        showStreamingLegSpacingEditor(hwnd, state);
        return true;
    }
    if (PtInRect(&layout.boxRect, point)) {
        state.streamingTargetDropdownOpen = false;
        closeStreamingHostEditor(hwnd, state);
        closeStreamingPortEditor(hwnd, state);
        closeStreamingSpacingEditors(hwnd, state);
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }
    return false;
}

} // namespace ovtr::win32
