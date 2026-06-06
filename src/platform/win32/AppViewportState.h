#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/ConfigTypes.h"
#include "platform/win32/ViewportGpuCapabilities.h"
#include "platform/win32/ViewportGpuMesh.h"
#include "platform/win32/ViewportMatcapShader.h"
#include "platform/win32/ViewportPaneTypes.h"
#include "platform/win32/ViewportRenderModelTypes.h"
#include "platform/win32/ViewportTriangleDisplayListCache.h"
#include "platform/win32/Win32GlContextResource.h"
#include "platform/win32/Win32GlDisplayListResource.h"
#include "platform/win32/Win32GlTextureResource.h"
#include "platform/win32/Win32UniqueWindowDcResource.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace ovtr::win32 {

inline constexpr float kDefaultCameraYawDegrees = 0.0f;
inline constexpr float kDefaultCameraPitchDegrees = 18.0f;
inline constexpr float kDefaultCameraDistance = 5.5f;
inline constexpr float kDefaultCameraPanX = 0.0f;
inline constexpr float kDefaultCameraPanY = 0.0f;
inline constexpr float kDefaultCameraPanZ = 0.0f;

struct AppViewportState {
    HWND glWindow = nullptr;
    UniqueWindowDc glDeviceContext;
    UniqueGlContext glContext;
    UniqueGlDisplayList glLabelFontBase;
    UniqueGlDisplayList glActorLabelFontBase;
    UniqueGlDisplayList glRecordingElapsedFontBase;
    UniqueGlDisplayList glOverlayFontBase;
    double targetViewportFps = 90.0;
    float cameraYawDegrees = kDefaultCameraYawDegrees;
    float cameraPitchDegrees = kDefaultCameraPitchDegrees;
    float cameraDistance = kDefaultCameraDistance;
    float cameraPanX = kDefaultCameraPanX;
    float cameraPanY = kDefaultCameraPanY;
    float cameraPanZ = kDefaultCameraPanZ;
    bool deviceLabelsVisible = true;
    bool quadViewEnabled = false;
    ViewportPaneKind activeDragPane = ViewportPaneKind::None;
    float frontViewPanX = 0.0f;
    float frontViewPanY = 0.0f;
    float frontViewZoom = kDefaultOrthoViewZoom;
    float topViewPanX = 0.0f;
    float topViewPanZ = 0.0f;
    float topViewZoom = kDefaultOrthoViewZoom;
    float leftViewPanZ = 0.0f;
    float leftViewPanY = 0.0f;
    float leftViewZoom = kDefaultOrthoViewZoom;
    bool orbitDragging = false;
    bool panDragging = false;
    int lastMouseX = 0;
    int lastMouseY = 0;
    ViewportSettings viewportSettings;
    std::unordered_map<std::string, RenderModelMesh> renderModelCache;
    std::vector<ViewportTriangleDisplayListCache> importedSceneMeshDisplayLists;
    std::vector<ViewportGpuMesh> importedSceneGpuMeshes;
    UniqueGlTexture renderModelMatcapTexture;
    bool renderModelMatcapTextureFailed = false;
    ViewportGpuCapabilities gpuCapabilities;
    ViewportMatcapShaderState matcapShader;
};

inline void toggleQuadView(AppViewportState& state) noexcept
{
    state.quadViewEnabled = !state.quadViewEnabled;
    state.orbitDragging = false;
    state.panDragging = false;
    state.activeDragPane = ViewportPaneKind::None;
}

} // namespace ovtr::win32
