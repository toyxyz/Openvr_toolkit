#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportGlStateScope.h"

namespace ovtr::win32 {

void drawGroundGrid3D(const RgbColor gridColor)
{
    ScopedGlLineWidth lineWidth(1.0f);
    setGlColor(gridColor);
    glBegin(GL_LINES);
    for (int i = -10; i <= 10; ++i) {
        const float value = static_cast<float>(i) * 0.5f;
        glVertex3f(value, 0.0f, -5.0f);
        glVertex3f(value, 0.0f, 5.0f);
        glVertex3f(-5.0f, 0.0f, value);
        glVertex3f(5.0f, 0.0f, value);
    }
    glEnd();
}

void drawAxes3D()
{
    constexpr float axisLength = 0.35f;

    ScopedGlLineWidth lineWidth(1.25f);
    glBegin(GL_LINES);
    glColor3f(0.55f, 0.16f, 0.16f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(axisLength, 0.0f, 0.0f);
    glColor3f(0.18f, 0.55f, 0.22f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, axisLength, 0.0f);
    glColor3f(0.20f, 0.30f, 0.65f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, axisLength);
    glEnd();
}

} // namespace ovtr::win32
