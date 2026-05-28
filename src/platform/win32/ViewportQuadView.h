#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/ViewportMath.h"
#include "platform/win32/ViewportPaneTypes.h"

namespace ovtr::win32 {

struct QuadViewLayout {
    RECT perspectiveRect{0, 0, 0, 0};
    RECT frontRect{0, 0, 0, 0};
    RECT topRect{0, 0, 0, 0};
    RECT leftRect{0, 0, 0, 0};
    bool valid = false;
};

QuadViewLayout quadViewLayoutForViewport(int width, int height) noexcept;
RECT rectForQuadViewPane(const QuadViewLayout& layout, ViewportPaneKind pane) noexcept;
ViewportPaneKind quadViewPaneFromPoint(const QuadViewLayout& layout, int x, int y) noexcept;
const char* quadViewPaneLabel(ViewportPaneKind pane) noexcept;
Vec3 orthoPanePanOffset(ViewportPaneKind pane, int dx, int dy, float worldUnitsPerPixel) noexcept;
float clampOrthoViewZoom(float zoom) noexcept;
float orthoViewZoomAfterWheel(float currentZoom, float wheelSteps) noexcept;

} // namespace ovtr::win32
