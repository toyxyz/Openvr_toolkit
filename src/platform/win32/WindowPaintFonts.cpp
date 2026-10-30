#include "platform/win32/WindowPaintFonts.h"
#include "platform/win32/UiTheme.h"

namespace ovtr::win32 {
namespace {

UniqueFont createUiFont(const int height, const wchar_t* faceName)
{
    return UniqueFont(CreateFontW(
        height,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        faceName
    ));
}

} // namespace

WindowPaintFonts createWindowPaintFonts()
{
    WindowPaintFonts fonts;
    fonts.body = createUiFont(-13, ui::fontFamily);
    fonts.status = createUiFont(-12, ui::fontFamily);
    fonts.debug = createUiFont(-12, L"Consolas");
    return fonts;
}

} // namespace ovtr::win32
