#include "platform/win32/ViewportRenderModelMatcap.h"

#include "platform/win32/AppViewportState.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/ViewportGlTextureBindingScope.h"
#include "platform/win32/Win32WicImageLoader.h"

#include <array>
#include <cstddef>
#include <cmath>
#include <cstdint>

namespace ovtr::win32 {
namespace {

constexpr int kMatcapSize = 64;

float saturate(const float value) noexcept
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

std::uint8_t toByte(const float value) noexcept
{
    return static_cast<std::uint8_t>(saturate(value) * 255.0f + 0.5f);
}

std::array<std::uint8_t, kMatcapSize * kMatcapSize * 4> makeMatcapPixels() noexcept
{
    std::array<std::uint8_t, kMatcapSize * kMatcapSize * 4> pixels{};
    for (int y = 0; y < kMatcapSize; ++y) {
        for (int x = 0; x < kMatcapSize; ++x) {
            const float nx = (static_cast<float>(x) + 0.5f) / static_cast<float>(kMatcapSize) * 2.0f - 1.0f;
            const float ny = (static_cast<float>(y) + 0.5f) / static_cast<float>(kMatcapSize) * 2.0f - 1.0f;
            const float radiusSquared = nx * nx + ny * ny;
            const float z = radiusSquared < 1.0f ? std::sqrt(1.0f - radiusSquared) : 0.0f;
            const float edgeFade = saturate(1.16f - std::sqrt(radiusSquared));
            const float base = 0.22f + 0.68f * z;
            const float highlight = std::pow(saturate(-nx * 0.34f + ny * 0.46f + z * 0.88f), 24.0f);
            const float rim = std::pow(saturate(1.0f - z), 2.6f) * edgeFade;

            const std::size_t offset = static_cast<std::size_t>((y * kMatcapSize + x) * 4);
            pixels[offset + 0] = toByte((0.24f + base * 0.60f + highlight * 0.70f + rim * 0.13f) * edgeFade);
            pixels[offset + 1] = toByte((0.30f + base * 0.68f + highlight * 0.68f + rim * 0.18f) * edgeFade);
            pixels[offset + 2] = toByte((0.40f + base * 0.76f + highlight * 0.64f + rim * 0.28f) * edgeFade);
            pixels[offset + 3] = 255;
        }
    }
    return pixels;
}

void uploadMatcapPixels(
    const GLuint texture,
    const GLsizei width,
    const GLsizei height,
    const std::uint8_t* pixels
) noexcept
{
    ScopedGlTexture2DBinding textureBinding(texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels
    );
}

bool uploadMatcapFileTexture(const GLuint texture) noexcept
{
    WicRgbaImage image;
    if (!loadRgbaImageWithWic(readableConfigPath(kRenderModelMatcapTextureFileName), image)) {
        return false;
    }
    uploadMatcapPixels(
        texture,
        static_cast<GLsizei>(image.width),
        static_cast<GLsizei>(image.height),
        image.pixels.data()
    );
    return true;
}

void uploadProceduralMatcapTexture(const GLuint texture) noexcept
{
    const auto pixels = makeMatcapPixels();
    uploadMatcapPixels(texture, kMatcapSize, kMatcapSize, pixels.data());
}

} // namespace

bool ensureRenderModelMatcapTexture(AppViewportState& state) noexcept
{
    if (state.renderModelMatcapTexture) {
        return true;
    }
    if (state.renderModelMatcapTextureFailed) {
        return false;
    }

    GLuint glTexture = 0;
    glGenTextures(1, &glTexture);
    if (glTexture == 0) {
        state.renderModelMatcapTextureFailed = true;
        return false;
    }

    UniqueGlTexture texture(glTexture);
    if (!uploadMatcapFileTexture(texture.get())) {
        uploadProceduralMatcapTexture(texture.get());
    }

    state.renderModelMatcapTexture.reset(texture.release());
    state.renderModelMatcapTextureFailed = false;
    return true;
}

void deleteRenderModelMatcapTexture(AppViewportState& state) noexcept
{
    state.renderModelMatcapTexture.reset();
    state.renderModelMatcapTextureFailed = false;
}

ScopedRenderModelMatcapMapping::ScopedRenderModelMatcapMapping() noexcept
    : wasSEnabled_(glIsEnabled(GL_TEXTURE_GEN_S) == GL_TRUE)
    , wasTEnabled_(glIsEnabled(GL_TEXTURE_GEN_T) == GL_TRUE)
{
    glGetTexGeniv(GL_S, GL_TEXTURE_GEN_MODE, &previousSMode_);
    glGetTexGeniv(GL_T, GL_TEXTURE_GEN_MODE, &previousTMode_);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
}

ScopedRenderModelMatcapMapping::~ScopedRenderModelMatcapMapping()
{
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, previousSMode_);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, previousTMode_);
    if (wasSEnabled_) {
        glEnable(GL_TEXTURE_GEN_S);
    } else {
        glDisable(GL_TEXTURE_GEN_S);
    }
    if (wasTEnabled_) {
        glEnable(GL_TEXTURE_GEN_T);
    } else {
        glDisable(GL_TEXTURE_GEN_T);
    }
}

} // namespace ovtr::win32
