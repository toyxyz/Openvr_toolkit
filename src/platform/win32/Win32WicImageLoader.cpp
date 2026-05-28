#include "platform/win32/Win32WicImageLoader.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincodec.h>

#include <algorithm>
#include <limits>
#include <utility>

namespace ovtr::win32 {
namespace {

template <typename T>
class ComPtr {
public:
    ComPtr() = default;

    ~ComPtr()
    {
        reset();
    }

    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;

    T* get() const noexcept
    {
        return pointer_;
    }

    T** put() noexcept
    {
        reset();
        return &pointer_;
    }

    T* operator->() const noexcept
    {
        return pointer_;
    }

    explicit operator bool() const noexcept
    {
        return pointer_ != nullptr;
    }

    void reset(T* pointer = nullptr) noexcept
    {
        if (pointer_ && pointer_ != pointer) {
            pointer_->Release();
        }
        pointer_ = pointer;
    }

private:
    T* pointer_ = nullptr;
};

class ScopedComInitializer {
public:
    ScopedComInitializer() noexcept
        : result_(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))
    {
    }

    ~ScopedComInitializer()
    {
        if (SUCCEEDED(result_)) {
            CoUninitialize();
        }
    }

    bool usable() const noexcept
    {
        return SUCCEEDED(result_) || result_ == RPC_E_CHANGED_MODE;
    }

    ScopedComInitializer(const ScopedComInitializer&) = delete;
    ScopedComInitializer& operator=(const ScopedComInitializer&) = delete;

private:
    HRESULT result_ = E_FAIL;
};

bool checkedImageByteCount(const UINT width, const UINT height, std::size_t& byteCount) noexcept
{
    constexpr std::size_t kBytesPerPixel = 4;
    const std::size_t maxSize = (std::numeric_limits<std::size_t>::max)();
    if (width == 0 ||
        height == 0 ||
        width > (std::numeric_limits<UINT>::max)() / kBytesPerPixel ||
        width > maxSize / height / kBytesPerPixel) {
        return false;
    }
    byteCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * kBytesPerPixel;
    return byteCount <= (std::numeric_limits<UINT>::max)();
}

void flipRowsInPlace(WicRgbaImage& image) noexcept
{
    const std::size_t stride = static_cast<std::size_t>(image.width) * 4;
    std::vector<std::uint8_t> row(stride);
    for (unsigned int y = 0; y < image.height / 2; ++y) {
        std::uint8_t* top = image.pixels.data() + static_cast<std::size_t>(y) * stride;
        std::uint8_t* bottom =
            image.pixels.data() + static_cast<std::size_t>(image.height - y - 1) * stride;
        std::copy(top, top + stride, row.data());
        std::copy(bottom, bottom + stride, top);
        std::copy(row.data(), row.data() + stride, bottom);
    }
}

} // namespace

bool loadRgbaImageWithWic(const std::filesystem::path& path, WicRgbaImage& image) noexcept
try {
    const ScopedComInitializer com;
    if (!com.usable()) {
        return false;
    }

    ComPtr<IWICImagingFactory> factory;
    HRESULT result = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(factory.put())
    );
    if (FAILED(result) || !factory) {
        return false;
    }

    ComPtr<IWICBitmapDecoder> decoder;
    result = factory->CreateDecoderFromFilename(
        path.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        decoder.put()
    );
    if (FAILED(result) || !decoder) {
        return false;
    }

    ComPtr<IWICBitmapFrameDecode> frame;
    result = decoder->GetFrame(0, frame.put());
    if (FAILED(result) || !frame) {
        return false;
    }

    UINT width = 0;
    UINT height = 0;
    result = frame->GetSize(&width, &height);
    std::size_t byteCount = 0;
    if (FAILED(result) || !checkedImageByteCount(width, height, byteCount)) {
        return false;
    }

    ComPtr<IWICFormatConverter> converter;
    result = factory->CreateFormatConverter(converter.put());
    if (FAILED(result) || !converter) {
        return false;
    }

    result = converter->Initialize(
        frame.get(),
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeCustom
    );
    if (FAILED(result)) {
        return false;
    }

    WicRgbaImage loaded;
    loaded.width = width;
    loaded.height = height;
    loaded.pixels.resize(byteCount);
    const UINT stride = width * 4;
    result = converter->CopyPixels(
        nullptr,
        stride,
        static_cast<UINT>(loaded.pixels.size()),
        loaded.pixels.data()
    );
    if (FAILED(result)) {
        return false;
    }

    flipRowsInPlace(loaded);
    image = std::move(loaded);
    return true;
} catch (...) {
    return false;
}

} // namespace ovtr::win32
