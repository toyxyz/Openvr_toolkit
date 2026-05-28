#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/ConfigTypes.h"
#include "platform/win32/ViewportRenderModelTypes.h"
#include "platform/win32/Win32GlContextResource.h"
#include "platform/win32/Win32GlDisplayListResource.h"
#include "platform/win32/Win32GlTextureResource.h"
#include "platform/win32/Win32UniqueWindowDcResource.h"

#include <string>
#include <unordered_map>

namespace ovtr::win32 {

struct AppViewportState {
    HWND glWindow = nullptr;
    UniqueWindowDc glDeviceContext;
    UniqueGlContext glContext;
    UniqueGlDisplayList glLabelFontBase;
    UniqueGlDisplayList glRecordingElapsedFontBase;
    UniqueGlDisplayList glOverlayFontBase;
    double targetViewportFps = 90.0;
    float cameraYawDegrees = 42.0f;
    float cameraPitchDegrees = 28.0f;
    float cameraDistance = 5.5f;
    float cameraPanX = 0.0f;
    float cameraPanY = 0.0f;
    float cameraPanZ = 0.0f;
    bool orbitDragging = false;
    bool panDragging = false;
    int lastMouseX = 0;
    int lastMouseY = 0;
    ViewportSettings viewportSettings;
    std::unordered_map<std::string, RenderModelMesh> renderModelCache;
    UniqueGlTexture renderModelMatcapTexture;
    bool renderModelMatcapTextureFailed = false;
};

} // namespace ovtr::win32
