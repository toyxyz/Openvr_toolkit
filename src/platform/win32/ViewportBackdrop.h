#pragma once

#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportGlMatrixScope.h"
#include "platform/win32/ViewportGlStateScope.h"

namespace ovtr::win32 {

inline void drawViewportBackdrop(const RgbColor base)
{
    setGlClearColor(base);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ScopedGlCapability depth(GL_DEPTH_TEST, false);
    ScopedGlCapability lighting(GL_LIGHTING, false);
    ScopedGlCapability texture(GL_TEXTURE_2D, false);
    ScopedGlCapability blend(GL_BLEND, false);
    ScopedGlCapability cull(GL_CULL_FACE, false);
    ScopedGlMatrixStack projection(GL_PROJECTION);
    glLoadIdentity();
    ScopedGlMatrixStack modelView(GL_MODELVIEW);
    glLoadIdentity();
    GLint shadeModel;
    glGetIntegerv(GL_SHADE_MODEL, &shadeModel);
    glShadeModel(GL_SMOOTH);
    glBegin(GL_QUADS);
    setGlColor(base);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    setGlColor({base.r * 4 / 5, base.g * 4 / 5, base.b * 4 / 5});
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();
    glShadeModel(static_cast<GLenum>(shadeModel));
}

} // namespace ovtr::win32
