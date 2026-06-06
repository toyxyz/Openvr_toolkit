#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <gl/GL.h>

#include "data/SessionTypes.h"
#include "platform/win32/ConfigTypes.h"

#include <array>
#include <string>

namespace ovtr::win32 {

void setGlColor(RgbColor color);
void setGlClearColor(RgbColor color);
void drawGroundGrid3D(RgbColor gridColor, float gridSize, float gridCellDensity);
void drawAxes3D();
void multiplyOpenGLMatrixFromQuaternion(const std::array<float, 4>& q);
void drawDeviceMarker3D(
    const ovtr::PoseSample& pose,
    ovtr::DeviceClass deviceClass,
    bool drawBody,
    bool selected
);
void offsetRasterPositionPixels(float x, float y);
void drawLabelText3D(const std::string& text, GLuint fontBase);

} // namespace ovtr::win32
