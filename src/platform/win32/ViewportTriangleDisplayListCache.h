#pragma once

#include "platform/win32/Win32GlDisplayListResource.h"

namespace ovtr::win32 {

struct ViewportTriangleDisplayListCache {
    UniqueGlDisplayList displayList;
    bool buildFailed = false;
};

void resetTriangleDisplayListCache(ViewportTriangleDisplayListCache& cache) noexcept;

} // namespace ovtr::win32
