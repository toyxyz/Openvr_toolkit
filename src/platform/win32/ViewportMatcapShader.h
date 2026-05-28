#pragma once

#include "platform/win32/Win32GlShaderResource.h"

#include <glad/glad.h>

#include <string>

namespace ovtr::win32 {

struct AppViewportState;

struct ViewportMatcapShaderState {
    UniqueGlProgram program;
    GLint positionAttrib = -1;
    GLint normalAttrib = -1;
    GLint mvpUniform = -1;
    GLint normalMatrixUniform = -1;
    GLint tintUniform = -1;
    GLint matcapUniform = -1;
    bool buildFailed = false;
    std::string failureReason;
};

const char* viewportMatcapVertexShaderSource() noexcept;
const char* viewportMatcapFragmentShaderSource() noexcept;
bool ensureMatcapShader(AppViewportState& state) noexcept;
void resetMatcapShader(ViewportMatcapShaderState& shader) noexcept;

} // namespace ovtr::win32
