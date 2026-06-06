#include "platform/win32/ViewportAnimationControlSections.h"

#include "platform/win32/AppImportedSceneState.h"
#include "platform/win32/AppLoadedSessionState.h"
#include "platform/win32/ImportedScenePlayback.h"
#include "platform/win32/SessionPlayback.h"

#include <sstream>

namespace ovtr::win32 {
namespace {

void drawAnimationFrameText(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const int currentFrame,
    const int totalFrames
)
{
    std::wostringstream frameText;
    frameText << L"Frame " << currentFrame << L" / " << totalFrames;
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

} // namespace

void drawImportedAnimationFrameText(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppImportedSceneState& state
)
{
    drawAnimationFrameText(drawDc, font, layout, importedSceneCurrentFrame(state), importedSceneTotalFrames(state));
}

void drawLoadedSessionAnimationFrameText(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppLoadedSessionState& state
)
{
    drawAnimationFrameText(drawDc, font, layout, loadedSessionCurrentFrame(state), loadedSessionTotalFrames(state));
}

} // namespace ovtr::win32
