#include "platform/win32/ViewportAnimationControlSections.h"

#include "platform/win32/AppImportedSceneState.h"
#include "platform/win32/ImportedScenePlayback.h"

#include <sstream>

namespace ovtr::win32 {

void drawImportedAnimationFrameText(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppImportedSceneState& state
)
{
    std::wostringstream frameText;
    frameText << L"Frame " << importedSceneCurrentFrame(state)
              << L" / " << importedSceneTotalFrames(state);
    if (font) {
        SelectObject(drawDc, font);
    }
    SetTextColor(drawDc, RGB(202, 211, 224));
    RECT frameTextRect = layout.frameTextRect;
    DrawTextW(
        drawDc,
        frameText.str().c_str(),
        -1,
        &frameTextRect,
        DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );
}

} // namespace ovtr::win32
