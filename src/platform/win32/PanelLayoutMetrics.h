#pragma once

namespace ovtr::win32 {

inline constexpr int kPanelContentMargin = 32;
inline constexpr int kPanelStatusBarHeight = 36;
inline constexpr int kPanelTopBarHeight = 32;
inline constexpr int kPanelDeviceToggleRailWidth = 32;
inline constexpr int kPanelDeviceToggleButtonWidth = 24;
inline constexpr int kPanelDeviceToggleButtonHeight = 96;
inline constexpr int kPanelSessionToggleButtonHeight = 112;
inline constexpr int kPanelProfileToggleRailWidth = 32;
inline constexpr int kPanelProfileToggleButtonWidth = 24;
inline constexpr int kPanelProfileToggleButtonHeight = 112;
inline constexpr int kPanelMappingToggleButtonHeight = 112;
inline constexpr int kPanelEditToggleButtonHeight = 64;
inline constexpr int kPanelProfileSplitterWidth = 8;
inline constexpr int kPanelProfilePanelDefaultWidth = 320;
inline constexpr int kPanelProfilePanelMinWidth = 260;
inline constexpr int kPanelProfilePanelMaxWidth = 720;
inline constexpr int kPanelSplitterWidth = 8;
inline constexpr int kLeftPanelMinWidth = 320;
inline constexpr int kLeftPanelMaxWidth = 880;
inline constexpr float kDefaultLeftPanelWidthRatio = 0.28f;
inline constexpr int kDefaultLeftPanelMinWidth = 420;
inline constexpr int kDefaultLeftPanelMaxWidth = 620;
inline constexpr int kViewportMinWidth = 320;

inline int panelStatusBarTopForClient(const int clientHeight) noexcept
{
    return clientHeight > kPanelStatusBarHeight
        ? clientHeight - kPanelStatusBarHeight
        : 0;
}

} // namespace ovtr::win32
