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
    constexpr float axisLift = 0.003f;

    // Lift the origin axes just above the grid plane to avoid line depth fighting.
    ScopedGlLineWidth lineWidth(2.5f);
    glBegin(GL_LINES);
    glColor3f(0.95f, 0.12f, 0.10f);
    glVertex3f(0.0f, axisLift, 0.0f);
    glVertex3f(axisLength, axisLift, 0.0f);
    glColor3f(0.20f, 0.85f, 0.28f);
    glVertex3f(0.0f, axisLift, 0.0f);
    glVertex3f(0.0f, axisLength + axisLift, 0.0f);
    glColor3f(0.28f, 0.44f, 1.0f);
    glVertex3f(0.0f, axisLift, 0.0f);
    glVertex3f(0.0f, axisLift, axisLength);
    glEnd();
}

} // namespace ovtr::win32
