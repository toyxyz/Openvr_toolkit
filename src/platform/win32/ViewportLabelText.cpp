#include "platform/win32/ViewportDrawPrimitives.h"

namespace ovtr::win32 {

void offsetRasterPositionPixels(const float x, const float y)
{
    glBitmap(0, 0, 0.0f, 0.0f, x, y, nullptr);
}

void drawLabelText3D(const std::string& text, const GLuint fontBase)
{
    if (fontBase == 0 || text.empty()) {
        return;
    }

    std::string ascii;
    ascii.reserve(text.size());
    for (const char ch : text) {
        const unsigned char value = static_cast<unsigned char>(ch);
        ascii.push_back(value >= 32 && value < 128 ? ch : '?');
    }

    glListBase(fontBase);
    glCallLists(static_cast<GLsizei>(ascii.size()), GL_UNSIGNED_BYTE, ascii.data());
}

} // namespace ovtr::win32
