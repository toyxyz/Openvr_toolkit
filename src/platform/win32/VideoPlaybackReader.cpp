#include "platform/win32/VideoPlaybackService.h"
#include "platform/win32/VideoPlaybackServiceInternal.h"

#include <mfapi.h>

#include <cstdint>
#include <limits>

namespace ovtr::win32 {

std::string videoHresultMessage(const char* operation, HRESULT result);

bool VideoPlaybackService::Implementation::openReader(
    const std::filesystem::path& path,
    std::string& outError
)
{
    std::error_code filesystemError;
    if (!std::filesystem::is_regular_file(path, filesystemError)) {
        outError = filesystemError
            ? "Inspecting session video failed: " + filesystemError.message()
            : "Session video file is missing.";
        return false;
    }
    Microsoft::WRL::ComPtr<IMFAttributes> attributes;
    HRESULT result = MFCreateAttributes(&attributes, 2);
    if (SUCCEEDED(result)) result = attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    if (SUCCEEDED(result)) {
        result = attributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
    }
    Microsoft::WRL::ComPtr<IMFSourceReader> openedReader;
    if (SUCCEEDED(result)) {
        result = MFCreateSourceReaderFromURL(path.c_str(), attributes.Get(), &openedReader);
    }

    Microsoft::WRL::ComPtr<IMFMediaType> outputType;
    if (SUCCEEDED(result)) result = MFCreateMediaType(&outputType);
    if (SUCCEEDED(result)) result = outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (SUCCEEDED(result)) result = outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    if (SUCCEEDED(result)) {
        result = openedReader->SetCurrentMediaType(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outputType.Get());
    }
    Microsoft::WRL::ComPtr<IMFMediaType> activeType;
    if (SUCCEEDED(result)) {
        result = openedReader->GetCurrentMediaType(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM, &activeType);
    }
    PROPVARIANT durationValue;
    PropVariantInit(&durationValue);
    UINT64 duration100ns = 0;
    HRESULT durationResult = openedReader
        ? openedReader->GetPresentationAttribute(
            MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &durationValue)
        : E_POINTER;
    if (SUCCEEDED(durationResult) && durationValue.vt == VT_UI8) {
        duration100ns = durationValue.uhVal.QuadPart;
    } else {
        durationResult = E_INVALIDARG;
    }
    PropVariantClear(&durationValue);
    UINT32 frameWidth = 0;
    UINT32 frameHeight = 0;
    if (SUCCEEDED(result)) {
        result = MFGetAttributeSize(activeType.Get(), MF_MT_FRAME_SIZE, &frameWidth, &frameHeight);
    }
    UINT32 frameRateNumerator = 0;
    UINT32 frameRateDenominator = 0;
    if (SUCCEEDED(result)) {
        const HRESULT frameRateResult = MFGetAttributeRatio(
            activeType.Get(), MF_MT_FRAME_RATE, &frameRateNumerator, &frameRateDenominator);
        if (SUCCEEDED(frameRateResult) && frameRateNumerator > 0 && frameRateDenominator > 0) {
            frameDurationSeconds =
                static_cast<double>(frameRateDenominator) / static_cast<double>(frameRateNumerator);
        }
    }
    if (FAILED(result) || frameWidth == 0 || frameHeight == 0) {
        outError = videoHresultMessage("Opening session video", result);
        return false;
    }
    const std::int64_t tightStride = static_cast<std::int64_t>(frameWidth) * 4;
    if (tightStride > (std::numeric_limits<std::int32_t>::max)()) {
        outError = "Decoded RGB32 video width exceeds the supported stride range.";
        return false;
    }
    UINT32 rawStride = 0;
    const HRESULT strideResult =
        activeType->GetUINT32(MF_MT_DEFAULT_STRIDE, &rawStride);
    const std::int32_t signedStride = SUCCEEDED(strideResult)
        ? static_cast<std::int32_t>(rawStride)
        : static_cast<std::int32_t>(tightStride);
    const std::int64_t absoluteStride = signedStride < 0
        ? -static_cast<std::int64_t>(signedStride)
        : static_cast<std::int64_t>(signedStride);
    if (absoluteStride != tightStride ||
        absoluteStride > (std::numeric_limits<std::uint32_t>::max)()) {
        outError = "Decoded RGB32 video reported an unsupported row stride.";
        return false;
    }
    width = frameWidth;
    height = frameHeight;
    decodedStride = static_cast<std::uint32_t>(absoluteStride);
    decodedTopDown = signedStride >= 0;
    durationSeconds = SUCCEEDED(durationResult) && duration100ns > 0
        ? static_cast<double>(duration100ns) / 10'000'000.0
        : 0.0;
    reachedEndOfStream = false;
    {
        std::lock_guard<std::mutex> lock(readerMutex);
        reader = std::move(openedReader);
    }
    return true;
}

} // namespace ovtr::win32
