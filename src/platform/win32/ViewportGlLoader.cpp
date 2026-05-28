#include "platform/win32/ViewportGlLoader.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"

#include <glad/glad.h>

namespace ovtr::win32 {

void initializeViewportGlLoader(AppWindowState& state) noexcept
{
    state.gpuCapabilities = {};
    if (gladLoadGL() == 0) {
        state.gpuCapabilities.failureReason = "gladLoadGL failed";
        appendDebugLog(state, "OpenGL GPU surface path disabled: " + state.gpuCapabilities.failureReason);
        return;
    }

    state.gpuCapabilities.gladLoaded = true;
    state.gpuCapabilities.shaderVboAvailable =
        glad_glGenBuffers != nullptr &&
        glad_glCreateShader != nullptr &&
        glad_glCreateProgram != nullptr &&
        glad_glVertexAttribPointer != nullptr;

    if (!state.gpuCapabilities.shaderVboAvailable) {
        state.gpuCapabilities.failureReason = "required VBO/shader functions are unavailable";
        appendDebugLog(state, "OpenGL GPU surface path disabled: " + state.gpuCapabilities.failureReason);
        return;
    }

    appendDebugLog(state, L"OpenGL GPU surface path enabled");
}

} // namespace ovtr::win32
