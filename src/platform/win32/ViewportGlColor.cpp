#include "platform/win32/ViewportDrawPrimitives.h"

#include "platform/win32/ConfigStore.h"

namespace ovtr::win32 {

void setGlColor(const RgbColor color)
{
    const RgbColor clamped = clampRgbColor(color);
    glColor3f(
        static_cast<float>(clamped.r) / 255.0f,
        static_cast<float>(clamped.g) / 255.0f,
        static_cast<float>(clamped.b) / 255.0f
    );
}

void setGlClearColor(const RgbColor color)
{
    const RgbColor clamped = clampRgbColor(color);
    glClearColor(
        static_cast<float>(clamped.r) / 255.0f,
        static_cast<float>(clamped.g) / 255.0f,
        static_cast<float>(clamped.b) / 255.0f,
        1.0f
    );
}

} // namespace ovtr::win32
