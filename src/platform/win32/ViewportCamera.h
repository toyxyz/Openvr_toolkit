#pragma once

#include "platform/win32/ViewportMath.h"

namespace ovtr::win32 {

struct AppViewportState;
struct AppWindowState;

CameraView cameraViewFromState(const AppViewportState& state);
float cameraDepthForWorldPoint(const CameraView& view, Vec3 point);
float cameraDepthForWorldPoint(const AppViewportState& state, Vec3 point);
void applyScreenSpacePan(AppViewportState& state, int dx, int dy);
void applyCameraDolly(AppViewportState& state, float distance);

CameraView cameraViewFromState(const AppWindowState& state);
float cameraDepthForWorldPoint(const AppWindowState& state, Vec3 point);
float outlineExpansionForDepth(float cameraDepth, int viewportHeight, float multiplier);
float outlineExpansionForOrtho(float worldUnitsPerPixel, float multiplier);
void applyScreenSpacePan(AppWindowState& state, int dx, int dy);
void applyCameraDolly(AppWindowState& state, float distance);

} // namespace ovtr::win32
