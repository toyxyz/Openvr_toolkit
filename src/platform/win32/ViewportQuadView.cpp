#include "platform/win32/ViewportQuadView.h"

#include <cmath>

namespace ovtr::win32 {
namespace {

constexpr float kOrthoZoomStepFactor = 1.15f;

} // namespace

QuadViewLayout quadViewLayoutForViewport(const int width, const int height) noexcept
{
    QuadViewLayout layout;
    if (width <= 1 || height <= 1) {
        return layout;
    }

    const int midX = width / 2;
    const int midY = height / 2;
    layout.perspectiveRect = RECT{0, 0, midX, midY};
    layout.frontRect = RECT{midX, 0, width, midY};
    layout.topRect = RECT{0, midY, midX, height};
    layout.leftRect = RECT{midX, midY, width, height};
    layout.valid = midX > 0 && midY > 0;
    return layout;
}

RECT rectForQuadViewPane(const QuadViewLayout& layout, const ViewportPaneKind pane) noexcept
{
    switch (pane) {
    case ViewportPaneKind::Perspective:
        return layout.perspectiveRect;
    case ViewportPaneKind::Front:
        return layout.frontRect;
    case ViewportPaneKind::Top:
        return layout.topRect;
    case ViewportPaneKind::Left:
        return layout.leftRect;
    case ViewportPaneKind::None:
    default:
        return RECT{0, 0, 0, 0};
    }
}

ViewportPaneKind quadViewPaneFromPoint(const QuadViewLayout& layout, const int x, const int y) noexcept
{
    const POINT point{x, y};
    if (PtInRect(&layout.perspectiveRect, point)) {
        return ViewportPaneKind::Perspective;
    }
    if (PtInRect(&layout.frontRect, point)) {
        return ViewportPaneKind::Front;
    }
    if (PtInRect(&layout.topRect, point)) {
        return ViewportPaneKind::Top;
    }
    if (PtInRect(&layout.leftRect, point)) {
        return ViewportPaneKind::Left;
    }
    return ViewportPaneKind::None;
}

const char* quadViewPaneLabel(const ViewportPaneKind pane) noexcept
{
    switch (pane) {
    case ViewportPaneKind::Perspective:
        return "Perspective";
    case ViewportPaneKind::Front:
        return "Front";
    case ViewportPaneKind::Top:
        return "Top";
    case ViewportPaneKind::Left:
        return "Left";
    case ViewportPaneKind::None:
    default:
        return "";
    }
}

Vec3 orthoPanePanOffset(
    const ViewportPaneKind pane,
    const int dx,
    const int dy,
    const float worldUnitsPerPixel
) noexcept
{
    const float horizontal = -static_cast<float>(dx) * worldUnitsPerPixel;
    const float vertical = static_cast<float>(dy) * worldUnitsPerPixel;
    switch (pane) {
    case ViewportPaneKind::Front:
        return {horizontal, vertical, 0.0f};
    case ViewportPaneKind::Top:
        return {horizontal, 0.0f, -vertical};
    case ViewportPaneKind::Left:
        return {0.0f, vertical, horizontal};
    case ViewportPaneKind::Perspective:
    case ViewportPaneKind::None:
    default:
        return {};
    }
}

float clampOrthoViewZoom(const float zoom) noexcept
{
    if (zoom < kMinimumOrthoViewZoom) {
        return kMinimumOrthoViewZoom;
    }
    if (zoom > kMaximumOrthoViewZoom) {
        return kMaximumOrthoViewZoom;
    }
    return zoom;
}

float orthoViewZoomAfterWheel(const float currentZoom, const float wheelSteps) noexcept
{
    return clampOrthoViewZoom(
        clampOrthoViewZoom(currentZoom) * std::pow(kOrthoZoomStepFactor, wheelSteps)
    );
}

} // namespace ovtr::win32
