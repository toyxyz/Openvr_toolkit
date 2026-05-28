#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {

struct WindowPaintFonts {
    UniqueFont body;
    UniqueFont status;
    UniqueFont debug;

    HFONT bodyFont() const noexcept
    {
        return body.get();
    }

    HFONT statusFont() const noexcept
    {
        return status ? status.get() : body.get();
    }

    HFONT debugFont() const noexcept
    {
        return debug ? debug.get() : body.get();
    }

    HFONT debugOrStatusFont() const noexcept
    {
        return debug ? debug.get() : statusFont();
    }
};

WindowPaintFonts createWindowPaintFonts();

} // namespace ovtr::win32
