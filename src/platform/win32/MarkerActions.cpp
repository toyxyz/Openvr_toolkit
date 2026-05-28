#include "platform/win32/MarkerActions.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/MarkerList.h"
#include "platform/win32/MarkerPoseActions.h"
#include "platform/win32/PoseSamplingWorker.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/Win32String.h"

namespace ovtr::win32 {

bool addMarkerFromSelectedDevice(HWND hwnd, AppWindowState& state, const ovtr::DeviceDescriptor& device)
{
    const ovtr::PosePollResult poses = copyLatestPoseSnapshot(state);
    std::string statusMessage;
    if (!addMarkerFromDevicePose(state, state, poses, device.runtimeIndex, state.viewportSettings.markerSize, statusMessage)) {
        appendDebugLog(state, L"Add marker failed: " + widen(statusMessage));
        return false;
    }
    const SceneMarker& marker = state.markers.back();
    appendDebugLog(state, L"Marker added: " + widen(marker.name));
    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

bool renameSelectedMarker(HWND hwnd, AppWindowState& state)
{
    SceneMarker* marker = markerForId(state, state.selectedMarkerId);
    if (!marker) {
        return false;
    }

    std::wstring nameText;
    if (!promptForMarkerName(hwnd, widen(marker->name), widen(marker->name), nameText)) {
        appendDebugLog(state, L"Rename marker canceled");
        return false;
    }

    const std::string name = trimAscii(narrow(trimWide(nameText)));
    if (!renameMarker(state, marker->id, name)) {
        appendDebugLog(state, L"Rename marker ignored: empty marker name");
        return false;
    }

    appendDebugLog(state, L"Marker renamed: " + widen(name));
    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

bool deleteSelectedMarker(HWND hwnd, AppWindowState& state)
{
    const SceneMarker* marker = markerForId(state, state.selectedMarkerId);
    if (!marker) {
        return false;
    }
    const std::wstring name = widen(marker->name);
    if (!deleteMarker(state, marker->id)) {
        return false;
    }

    appendDebugLog(state, L"Marker deleted: " + name);
    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

} // namespace ovtr::win32
