#pragma once

#include "platform/win32/AppImportedSceneState.h"
#include "platform/win32/LayoutTypes.h"
#include "platform/win32/UiTheme.h"

namespace ovtr::win32 {

inline void drawImportedSceneInfoBar(HDC dc, HFONT font, const ViewportControlLayout& layout,
    const AppImportedSceneState& state)
{
    if (!state.importedSceneLoaded || !layout.importedSceneBarValid) return;
    ui::fill(dc, layout.importedSceneBarRect, ui::header);
    SelectObjectGuard fontSelection(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, ui::text);
    const auto label = L"GLB  " + state.importedScene.sourcePath.filename().wstring();
    RECT text = layout.importedSceneLabelRect;
    DrawTextW(dc, label.c_str(), -1, &text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    ui::buttonSurface(dc, layout.importedSceneCloseButtonRect);
    text = layout.importedSceneCloseButtonRect;
    DrawTextW(dc, L"Close", -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

} // namespace ovtr::win32
