#include "platform/win32/ViewportRenderer.h"

#include "platform/win32/AppState.h"
#include "platform/win32/ViewportGlFonts.h"
#include "platform/win32/ViewportGlLoader.h"
#include "platform/win32/ViewportRenderModelCleanup.h"
#include "platform/win32/Win32GlContextResource.h"
#include "platform/win32/Win32ScopedDcResources.h"
#include "platform/win32/Win32UniqueWindowDcResource.h"

namespace ovtr::win32 {

bool setupOpenGLForChild(HWND hwnd, AppWindowState& state) noexcept
{
    WindowDc deviceContext(hwnd);
    if (!deviceContext) {
        return false;
    }

    PIXELFORMATDESCRIPTOR descriptor{};
    descriptor.nSize = sizeof(descriptor);
    descriptor.nVersion = 1;
    descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    descriptor.iPixelType = PFD_TYPE_RGBA;
    descriptor.cColorBits = 32;
    descriptor.cDepthBits = 24;
    descriptor.cStencilBits = 8;
    descriptor.iLayerType = PFD_MAIN_PLANE;

    const int pixelFormat = ChoosePixelFormat(deviceContext.get(), &descriptor);
    if (pixelFormat == 0) {
        return false;
    }

    if (!SetPixelFormat(deviceContext.get(), pixelFormat, &descriptor)) {
        return false;
    }

    UniqueGlContext glContext(wglCreateContext(deviceContext.get()));
    if (!glContext) {
        return false;
    }

    if (!wglMakeCurrent(deviceContext.get(), glContext.get())) {
        return false;
    }

    state.glDeviceContext.reset(hwnd, deviceContext.release());
    state.glContext.reset(glContext.release());

    initializeViewportGlLoader(state);
    createViewportGlFontDisplayLists(state);
    return true;
}

void shutdownOpenGLForChild(AppWindowState& state) noexcept
{
    if (state.glContext) {
        wglMakeCurrent(state.glDeviceContext.get(), state.glContext.get());
        deleteRenderModelTextures(state);
        state.glLabelFontBase.reset();
        state.glRecordingElapsedFontBase.reset();
        state.glOverlayFontBase.reset();
        wglMakeCurrent(nullptr, nullptr);
        state.glContext.reset();
    }

    state.glDeviceContext.reset();
}

} // namespace ovtr::win32
