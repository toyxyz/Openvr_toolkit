#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/ViewportCamera.h"

#include <string>

namespace ovtr::win32 {

struct AppWindowState;
struct RenderModelMesh;

bool setupOpenGLForChild(HWND hwnd, AppWindowState& state) noexcept;
void shutdownOpenGLForChild(AppWindowState& state) noexcept;
bool updateRenderModelMesh(RenderModelMesh& mesh, const std::string& renderModelName);
void renderViewport(HWND hwnd);

} // namespace ovtr::win32
