#pragma once

#include "platform/win32/AppDebugUiState.h"
#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppImportedSceneState.h"
#include "platform/win32/AppOriginState.h"
#include "platform/win32/AppPoseSamplingState.h"
#include "platform/win32/AppRecordingState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/AppTopBarState.h"
#include "platform/win32/AppViewportState.h"

namespace ovtr::win32 {

struct AppWindowState
    : AppRuntimeState
    , AppPoseSamplingState
    , AppViewportState
    , AppRecordingState
    , AppOriginState
    , AppImportedSceneState
    , AppDeviceState
    , AppTopBarState
    , AppDebugUiState {
};

} // namespace ovtr::win32
