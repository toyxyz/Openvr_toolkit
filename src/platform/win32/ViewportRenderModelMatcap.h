#pragma once

#include "platform/win32/Win32GlTextureResource.h"

#include <gl/GL.h>

namespace ovtr::win32 {

struct AppViewportState;

bool ensureRenderModelMatcapTexture(AppViewportState& state) noexcept;
void deleteRenderModelMatcapTexture(AppViewportState& state) noexcept;

class ScopedRenderModelMatcapMapping {
public:
    ScopedRenderModelMatcapMapping() noexcept;
    ~ScopedRenderModelMatcapMapping();

    ScopedRenderModelMatcapMapping(const ScopedRenderModelMatcapMapping&) = delete;
    ScopedRenderModelMatcapMapping& operator=(const ScopedRenderModelMatcapMapping&) = delete;

private:
    GLint previousSMode_ = GL_OBJECT_LINEAR;
    GLint previousTMode_ = GL_OBJECT_LINEAR;
    bool wasSEnabled_ = false;
    bool wasTEnabled_ = false;
};

} // namespace ovtr::win32
