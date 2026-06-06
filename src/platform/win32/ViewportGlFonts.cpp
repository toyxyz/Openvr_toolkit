#include "platform/win32/ViewportGlFonts.h"

#include "platform/win32/AppState.h"
#include "platform/win32/Win32GdiResources.h"

#include <gl/GL.h>

namespace ovtr::win32 {
namespace {

constexpr int kLabelFontHeight = 13;
constexpr int kActorLabelFontHeight = kLabelFontHeight * 2;
constexpr int kRecordingElapsedFontHeight = kLabelFontHeight * 2;
constexpr int kDelayCountdownFontHeight = 54;

void createGlFontDisplayList(
    UniqueGlDisplayList& displayList,
    HDC deviceContext,
    const int height,
    const int weight
)
{
    displayList.reset(glGenLists(128), 128);
    if (!displayList) {
        return;
    }

    UniqueFont font(CreateFontW(
        height,
        0,
        0,
        0,
        weight,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    ));
    SelectObjectGuard fontSelection(deviceContext, font.get());
    wglUseFontBitmapsW(deviceContext, 0, 128, displayList.get());
}

} // namespace

void createViewportGlFontDisplayLists(AppWindowState& state)
{
    createGlFontDisplayList(state.glLabelFontBase, state.glDeviceContext.get(), -kLabelFontHeight, FW_SEMIBOLD);
    createGlFontDisplayList(
        state.glActorLabelFontBase,
        state.glDeviceContext.get(),
        -kActorLabelFontHeight,
        FW_SEMIBOLD
    );
    createGlFontDisplayList(
        state.glRecordingElapsedFontBase,
        state.glDeviceContext.get(),
        -kRecordingElapsedFontHeight,
        FW_BOLD
    );
    createGlFontDisplayList(
        state.glOverlayFontBase,
        state.glDeviceContext.get(),
        -kDelayCountdownFontHeight,
        FW_BOLD
    );
}

} // namespace ovtr::win32
