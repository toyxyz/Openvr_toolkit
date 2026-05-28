#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/Layout.h"

#include <cstdint>

namespace ovtr::win32 {

struct AppDeviceState;
struct AppMarkerState;
struct AppRuntimeState;
struct AppWindowState;

int activeDebugMonitorHeight(const AppWindowState* state, int clientHeight);
RECT deviceToggleButtonRectForClient(const AppWindowState* state, int clientWidth, int clientHeight);
RECT splitterRectForClient(const AppWindowState* state, int clientWidth, int clientHeight);
int leftPanelContentBottomForClient(const AppWindowState* state, int clientHeight);
ViewportControlLayout viewportControlLayoutForClient(
    const AppWindowState* state,
    int clientWidth,
    int clientHeight
);
RECT viewportRenderRectForClient(const AppWindowState* state, int clientWidth, int clientHeight);
OriginPanelLayout originPanelLayoutForClient(
    const AppWindowState* state,
    int clientWidth,
    int clientHeight
);
RECT originEditorRectForClient(const AppWindowState* state, int clientWidth, int clientHeight);
OriginStepperButton originStepperButtonFromPoint(
    const AppWindowState* state,
    int clientWidth,
    int clientHeight,
    POINT point
);
DeviceListLayout deviceListLayoutForClient(const AppWindowState* state, int clientWidth, int clientHeight);
MarkerListLayout markerListLayoutForClient(const AppWindowState* state, int clientWidth, int clientHeight);
RECT debugMessagesRectForClient(const AppWindowState* state, int clientWidth, int clientHeight);
RECT debugInfoRectForClient(const AppWindowState* state, int clientWidth, int clientHeight);
RECT debugResizeRectForClient(const AppWindowState* state, int clientWidth, int clientHeight);

int visibleDebugLogLineCount(const RECT& messagesRect);
int maxDebugLogScrollOffset(const AppWindowState& state, int visibleLineCount);
void clampDebugLogScroll(AppWindowState& state, int visibleLineCount);
int maxDebugInfoScrollOffset(const AppWindowState& state, int visibleLineCount);
void clampDebugInfoScroll(AppWindowState& state, int visibleLineCount);
int maxDeviceListScrollOffset(const AppWindowState& state, int visibleItemCount);
void clampDeviceListScroll(AppWindowState& state, int visibleItemCount);
int maxMarkerListScrollOffset(const AppWindowState& state, int visibleItemCount);
void clampMarkerListScroll(AppWindowState& state, int visibleItemCount);
std::uint32_t deviceRuntimeIndexFromListPoint(
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState,
    const DeviceListLayout& layout,
    POINT point
);
std::uint32_t deviceRuntimeIndexFromListPoint(
    const AppWindowState& state,
    const DeviceListLayout& layout,
    POINT point
);
std::uint32_t markerIdFromListPoint(
    const AppWindowState& state,
    const MarkerListLayout& layout,
    POINT point
);

int leftPanelWidthForClient(const AppWindowState* state, int clientWidth);
void updateOriginEditorLayout(HWND hwnd, AppWindowState& state);
void layoutChildWindows(HWND hwnd);
void invalidateWindowLayout(HWND hwnd);
void invalidateStatusPanel(HWND hwnd);

} // namespace ovtr::win32
