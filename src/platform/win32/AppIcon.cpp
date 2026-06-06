#include "platform/win32/AppIcon.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/Win32String.h"
#include "platform/win32/Win32WicImageLoader.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::win32 {
namespace {

constexpr const char* kAppIconFileName = "icon.png";

HBITMAP createColorBitmapFromRgba(const WicRgbaImage& image)
{
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = static_cast<LONG>(image.width);
    info.bmiHeader.biHeight = static_cast<LONG>(image.height);
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bitmap || !bits) {
        return nullptr;
    }

    auto* dst = static_cast<std::uint8_t*>(bits);
    for (std::size_t i = 0; i < image.pixels.size(); i += 4) {
        dst[i + 0] = image.pixels[i + 2];
        dst[i + 1] = image.pixels[i + 1];
        dst[i + 2] = image.pixels[i + 0];
        dst[i + 3] = image.pixels[i + 3];
    }
    return bitmap;
}

HBITMAP createTransparentMaskBitmap(const WicRgbaImage& image)
{
    const int width = static_cast<int>(image.width);
    const int height = static_cast<int>(image.height);
    const int stride = ((width + 15) / 16) * 2;
    std::vector<std::uint8_t> mask(static_cast<std::size_t>(stride) * height, 0);
    return CreateBitmap(width, height, 1, 1, mask.data());
}

HICON createIconFromImage(const WicRgbaImage& image)
{
    HBITMAP color = createColorBitmapFromRgba(image);
    HBITMAP mask = createTransparentMaskBitmap(image);
    if (!color || !mask) {
        if (color) {
            DeleteObject(color);
        }
        if (mask) {
            DeleteObject(mask);
        }
        return nullptr;
    }

    ICONINFO iconInfo{};
    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = color;
    iconInfo.hbmMask = mask;
    HICON icon = CreateIconIndirect(&iconInfo);
    DeleteObject(color);
    DeleteObject(mask);
    return icon;
}

HICON copyIconSize(HICON source, const int width, const int height)
{
    return static_cast<HICON>(CopyImage(source, IMAGE_ICON, width, height, 0));
}

void replaceIcon(HICON& target, HICON icon) noexcept
{
    if (target) {
        DestroyIcon(target);
    }
    target = icon;
}

} // namespace

void applyConfiguredAppIcon(HWND hwnd, AppWindowState& state)
{
    const std::filesystem::path iconPath = readableConfigPath(kAppIconFileName);
    WicRgbaImage image;
    if (!loadRgbaImageWithWic(iconPath, image)) {
        appendDebugLog(state, L"App icon load skipped: " + iconPath.wstring());
        return;
    }

    HICON loadedIcon = createIconFromImage(image);
    if (!loadedIcon) {
        appendDebugLog(state, L"App icon conversion failed: " + iconPath.wstring());
        return;
    }

    HICON bigIcon = copyIconSize(loadedIcon, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    HICON smallIcon =
        copyIconSize(loadedIcon, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    if (!bigIcon) {
        bigIcon = loadedIcon;
        loadedIcon = nullptr;
    }
    if (!smallIcon) {
        smallIcon = copyIconSize(bigIcon, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    }
    if (loadedIcon) {
        DestroyIcon(loadedIcon);
    }

    replaceIcon(state.appIconBig, bigIcon);
    replaceIcon(state.appIconSmall, smallIcon);
    if (state.appIconBig) {
        SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(state.appIconBig));
    }
    if (state.appIconSmall) {
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(state.appIconSmall));
    }
    appendDebugLog(state, L"App icon loaded: " + iconPath.wstring());
}

void destroyConfiguredAppIcon(AppWindowState& state) noexcept
{
    replaceIcon(state.appIconBig, nullptr);
    replaceIcon(state.appIconSmall, nullptr);
}

} // namespace ovtr::win32
