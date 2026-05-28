#pragma once

namespace ovtr::win32 {

enum class ViewportPaneKind {
    None,
    Perspective,
    Front,
    Top,
    Left,
};

inline constexpr float kDefaultOrthoViewZoom = 1.0f;
inline constexpr float kMinimumOrthoViewZoom = 0.125f;
inline constexpr float kMaximumOrthoViewZoom = 32.0f;

} // namespace ovtr::win32
