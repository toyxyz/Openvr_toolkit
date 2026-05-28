#include "platform/win32/OriginEditorActions.h"

#include "platform/win32/AppConfig.h"
#include "platform/win32/AppState.h"
#include "platform/win32/AppStateConstants.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/OriginEditor.h"
#include "platform/win32/OriginState.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/WindowLayout.h"

#include <array>
#include <mutex>
#include <string>

namespace ovtr::win32 {

bool applyOriginEditorText(HWND hwnd, AppWindowState& state)
{
    if (!state.originEditWindow || !IsWindow(state.originEditWindow)) {
        return false;
    }

    std::array<float, 3> offset{};
    std::array<float, 3> rotation{};
    if (!parseOriginEditorText(readWindowText(state.originEditWindow), offset, rotation)) {
        state.originStatusMessage = "origin edit requires 6 values: x y z rx ry rz";
        appendDebugLog(state, state.originStatusMessage);
        InvalidateRect(hwnd, nullptr, FALSE);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(state.originMutex);
        state.originOffset = offset;
        state.originRotationDegrees = rotation;
        state.selectedOriginRuntimeIndex = kNoSelectedRuntimeIndex;
        if (originValuesAreZero(offset, rotation)) {
            state.originEnabled = false;
            state.originStatusMessage = "origin reset from editor";
        } else {
            state.originEnabled = true;
            state.originStatusMessage = "origin manually edited";
        }
    }

    appendDebugLog(state, state.originStatusMessage);
    saveOriginConfig(state);
    closeOriginEditor(hwnd, state);

    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    invalidateStatusPanel(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

void applyOriginStepperButton(HWND hwnd, AppWindowState& state, const OriginStepperButton& button)
{
    if (!button.valid || button.axis < 0 || button.axis >= 3) {
        return;
    }

    if (state.originEditWindow && IsWindow(state.originEditWindow)) {
        closeOriginEditor(hwnd, state);
    }

    if (!adjustOriginAxis(state, button.rotation, button.axis, button.delta)) {
        return;
    }
    appendDebugLog(state, state.originStatusMessage);
    saveOriginConfig(state);

    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    invalidateStatusPanel(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
}

} // namespace ovtr::win32
