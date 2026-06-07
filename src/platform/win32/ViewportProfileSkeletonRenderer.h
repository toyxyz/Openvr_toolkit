#pragma once

#include "platform/win32/AppDebugUiState.h"
#include "platform/win32/AppProfileState.h"
#include "platform/win32/AppOriginState.h"
#include "platform/win32/AppRecordingState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/AppStreamingState.h"
#include "platform/win32/AppViewportState.h"

#include <gl/GL.h>

namespace ovtr::win32 {

void drawProfileSkeletonPreview3D(
    const AppProfileState& profileState,
    const AppDebugUiState& debugState,
    AppViewportState& viewportState
);
void drawMappingActors3D(
    AppProfileState& profileState,
    const AppRuntimeState& runtimeState,
    const AppOriginState& originState,
    AppRecordingState& recordingState,
    AppStreamingState& streamingState,
    AppDebugUiState& debugState,
    AppViewportState& viewportState
);
void drawMappingActorLabels3D(const AppProfileState& profileState, GLuint fontBase);

} // namespace ovtr::win32
