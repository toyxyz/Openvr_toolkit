#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportGlStateScope.h"

#include <algorithm>
#include <cmath>

namespace ovtr::win32 {

void drawGroundGrid3D(
    const RgbColor gridColor,
    const float gridSize,
    const float gridCellDensity
)
{
    const float halfSize = std::clamp(gridSize, 1.0f, 50.0f);
    const float density = std::clamp(gridCellDensity, 0.25f, 10.0f);
    int lineCount = static_cast<int>(std::lround(halfSize * density));
    if (lineCount < 1) {
        lineCount = 1;
    }
    const float step = 1.0f / density;
    const float extent = static_cast<float>(lineCount) * step;

    ScopedGlLineWidth lineWidth(1.0f);
    setGlColor(gridColor);
    glBegin(GL_LINES);
    for (int i = -lineCount; i <= lineCount; ++i) {
        const float value = static_cast<float>(i) * step;
        glVertex3f(value, 0.0f, -extent);
        glVertex3f(value, 0.0f, extent);
        glVertex3f(-extent, 0.0f, value);
        glVertex3f(extent, 0.0f, value);
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
