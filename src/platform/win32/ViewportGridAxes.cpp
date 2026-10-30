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
        if (i == 0) continue; // The two colored floor axes occupy the center lines.
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
    constexpr float extent = 100.0f;
    constexpr float axisLift = 0.001f;

    // Blender-style floor guides in the recorder's Y-up coordinate system.
    // They span both sides of the origin; there is no vertical origin triad.
    ScopedGlLineWidth lineWidth(1.25f);
    glBegin(GL_LINES);
    setGlColor({147, 65, 77});
    glVertex3f(-extent, axisLift, 0.0f);
    glVertex3f(extent, axisLift, 0.0f);
    setGlColor({93, 127, 56});
    glVertex3f(0.0f, axisLift, -extent);
    glVertex3f(0.0f, axisLift, extent);
    glEnd();
}

} // namespace ovtr::win32
