#include "platform/win32/WindowPaintFonts.h"

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
    fonts.body = createUiFont(17, L"Segoe UI");
    fonts.status = createUiFont(15, L"Segoe UI");
    fonts.debug = createUiFont(14, L"Consolas");
    return fonts;
}

} // namespace ovtr::win32
