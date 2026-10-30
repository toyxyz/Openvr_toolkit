#include "platform/win32/VideoCaptureService.h"
#include "platform/win32/VideoCaptureServiceInternal.h"

#include <mfapi.h>
#include <mferror.h>

#include <cstdint>
#include <limits>

namespace ovtr::win32 {
namespace {

Microsoft::WRL::ComPtr<IMFActivate> findCamera(const std::wstring& symbolicLink, std::string& error)
{
    Microsoft::WRL::ComPtr<IMFAttributes> attributes;
    HRESULT result = MFCreateAttributes(&attributes, 1);
    if (SUCCEEDED(result)) {
        result = attributes->SetGUID(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
        );
    }
    if (FAILED(result)) {
        error = videoHresultMessage("Preparing camera selection", result);
        return {};
    }
    IMFActivate** activations = nullptr;
    UINT32 count = 0;
    result = MFEnumDeviceSources(attributes.Get(), &activations, &count);
    if (FAILED(result)) {
        error = videoHresultMessage("Enumerating cameras", result);
        return {};
    }
    Microsoft::WRL::ComPtr<IMFActivate> selected;
    for (UINT32 index = 0; index < count; ++index) {
        wchar_t* value = nullptr;
        UINT32 length = 0;
        if (SUCCEEDED(activations[index]->GetAllocatedString(
                MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &value, &length))) {
            if (value && std::wstring(value, length) == symbolicLink) {
                selected = activations[index];
            }
            CoTaskMemFree(value);
        }
        activations[index]->Release();
    }
    CoTaskMemFree(activations);
    if (!selected) {
        error = "The selected camera is no longer connected.";
    }
    return selected;
}

bool readVideoFormat(
    IMFSourceReader* reader,
    std::uint32_t& width,
    std::uint32_t& height,
    std::uint32_t& rateNumerator,
    std::uint32_t& rateDenominator,
    std::int32_t& stride,
    std::string& error
)
{
    Microsoft::WRL::ComPtr<IMFMediaType> type;
    HRESULT result = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &type);
    UINT32 frameWidth = 0;
    UINT32 frameHeight = 0;
    if (SUCCEEDED(result)) {
        result = MFGetAttributeSize(type.Get(), MF_MT_FRAME_SIZE, &frameWidth, &frameHeight);
    }
    if (FAILED(result) || frameWidth == 0 || frameHeight == 0) {
        error = videoHresultMessage("Reading camera frame size", result);
        return false;
    }
    UINT32 numerator = 0;
    UINT32 denominator = 0;
    result = MFGetAttributeRatio(type.Get(), MF_MT_FRAME_RATE, &numerator, &denominator);
    if (FAILED(result)) {
        error = videoHresultMessage("Reading camera frame rate", result);
        return false;
    }
    if (numerator == 0 || denominator == 0) {
        error = "Camera reported an invalid frame rate.";
        return false;
    }
    const std::uint64_t tightStride = static_cast<std::uint64_t>(frameWidth) * 4u;
    if (tightStride > static_cast<std::uint64_t>((std::numeric_limits<std::int32_t>::max)())) {
        error = "Camera frame width exceeds the supported RGB32 stride range.";
        return false;
    }
    UINT32 rawStride = 0;
    const HRESULT strideResult = type->GetUINT32(MF_MT_DEFAULT_STRIDE, &rawStride);
    const std::int32_t signedStride = SUCCEEDED(strideResult)
        ? static_cast<std::int32_t>(rawStride)
        : static_cast<std::int32_t>(tightStride);
    const std::int64_t absoluteStride = signedStride < 0
        ? -static_cast<std::int64_t>(signedStride)
        : static_cast<std::int64_t>(signedStride);
    if (absoluteStride < static_cast<std::int64_t>(tightStride)) {
        error = "Camera reported an RGB32 row stride smaller than its frame width.";
        return false;
    }
    width = frameWidth;
    height = frameHeight;
    rateNumerator = numerator;
    rateDenominator = denominator;
    stride = signedStride;
    return true;
}

} // namespace

bool VideoCaptureService::Implementation::openCamera(
    const std::wstring& symbolicLink,
    std::string& outError
)
{
    Microsoft::WRL::ComPtr<IMFActivate> activation = findCamera(symbolicLink, outError);
    if (!activation) {
        return false;
    }
    const char* operation = "Activating selected camera";
    Microsoft::WRL::ComPtr<IMFMediaSource> openedSource;
    Microsoft::WRL::ComPtr<IMFSourceReader> openedReader;
    HRESULT result = activation->ActivateObject(IID_PPV_ARGS(&openedSource));
    Microsoft::WRL::ComPtr<IMFAttributes> readerAttributes;
    if (SUCCEEDED(result)) {
        operation = "Creating camera reader settings";
        result = MFCreateAttributes(&readerAttributes, 2);
    }
    if (SUCCEEDED(result)) {
        operation = "Enabling camera hardware transforms";
        result = readerAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    }
    if (SUCCEEDED(result)) {
        operation = "Enabling advanced camera video processing";
        result = readerAttributes->SetUINT32(
            MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
    }
    if (SUCCEEDED(result)) {
        operation = "Creating camera source reader";
        result = MFCreateSourceReaderFromMediaSource(
            openedSource.Get(), readerAttributes.Get(), &openedReader);
    }
    Microsoft::WRL::ComPtr<IMFMediaType> outputType;
    if (SUCCEEDED(result)) {
        operation = "Creating RGB32 camera media type";
        result = MFCreateMediaType(&outputType);
    }
    if (SUCCEEDED(result)) {
        result = outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    }
    if (SUCCEEDED(result)) {
        result = outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    }
    if (SUCCEEDED(result)) {
        operation = "Selecting RGB32 camera output";
        result = openedReader->SetCurrentMediaType(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outputType.Get());
    }
    if (FAILED(result)) {
        outError = videoHresultMessage(operation, result);
        if (openedSource) openedSource->Shutdown();
        return false;
    }
    std::uint32_t openedWidth = 0;
    std::uint32_t openedHeight = 0;
    std::uint32_t openedRateNumerator = 0;
    std::uint32_t openedRateDenominator = 0;
    std::int32_t openedStride = 0;
    if (!readVideoFormat(
            openedReader.Get(),
            openedWidth,
            openedHeight,
            openedRateNumerator,
            openedRateDenominator,
            openedStride,
            outError)) {
        openedSource->Shutdown();
        return false;
    }
    bool canceled = false;
    {
        std::lock_guard<std::mutex> lock(sourceMutex);
        canceled = stopRequested.load(std::memory_order_acquire);
        if (!canceled) {
            mediaSource = std::move(openedSource);
            reader = std::move(openedReader);
            width = openedWidth;
            height = openedHeight;
            frameRateNumerator = openedRateNumerator;
            frameRateDenominator = openedRateDenominator;
            captureStride = openedStride;
        }
    }
    if (canceled) {
        openedSource->Shutdown();
        outError = "Camera activation was canceled.";
        return false;
    }
    return true;
}

} // namespace ovtr::win32
