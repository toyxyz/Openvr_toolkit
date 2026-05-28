#pragma once

namespace ovtr::win32 {

enum class ActiveTopBarMenu {
    None,
    File,
    Setting,
};

struct AppTopBarState {
    ActiveTopBarMenu activeTopBarMenu = ActiveTopBarMenu::None;
};

} // namespace ovtr::win32
