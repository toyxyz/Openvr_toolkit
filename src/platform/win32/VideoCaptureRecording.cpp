#include "platform/win32/VideoCaptureService.h"
#include "platform/win32/VideoCaptureServiceInternal.h"

#include <mfapi.h>

#include <filesystem>
#include <limits>

namespace ovtr::win32 {
namespace {

std::filesystem::path partialVideoPath(const std::filesystem::path& finalPath)
{
    const std::wstring name = finalPath.stem().wstring() +
        L".partial" + finalPath.extension().wstring();
    return finalPath.parent_path() / name;
}

bool removeRecordingTarget(const std::filesystem::path& path, std::string& outError)
{
    std::error_code error;
    std::filesystem::remove(path, error);
    if (!error) return true;
    outError = "Removing stale video recording failed: " + error.message();
    return false;
}

} // namespace

bool VideoCaptureService::Implementation::createWriter(
    const std::filesystem::path& path,
    std::string& outError
)
{
    if (writer) {
        outError = "Video recording is already active.";
        return false;
    }
    if (width == 0 || height == 0 ||
        width > static_cast<std::uint32_t>((std::numeric_limits<LONG>::max)() / 4)) {
        outError = "Camera frame dimensions cannot be encoded safely.";
        return false;
    }
    std::error_code filesystemError;
    std::filesystem::create_directories(path.parent_path(), filesystemError);
    if (filesystemError) {
        outError = "Creating video session directory failed: " + filesystemError.message();
        return false;
    }
    activeRecordingFinalPath = path;
    activeRecordingPartialPath = partialVideoPath(path);
    if (!removeRecordingTarget(activeRecordingFinalPath, outError) ||
        !removeRecordingTarget(activeRecordingPartialPath, outError)) {
        return false;
    }

    Microsoft::WRL::ComPtr<IMFAttributes> attributes;
    HRESULT result = MFCreateAttributes(&attributes, 2);
    if (SUCCEEDED(result)) {
        result = attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    }
    if (SUCCEEDED(result)) {
        result = attributes->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE);
    }
    if (SUCCEEDED(result)) {
        result = MFCreateSinkWriterFromURL(
            activeRecordingPartialPath.c_str(), nullptr, attributes.Get(), &writer);
    }

    Microsoft::WRL::ComPtr<IMFMediaType> outputType;
    if (SUCCEEDED(result)) result = MFCreateMediaType(&outputType);
    if (SUCCEEDED(result)) result = outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (SUCCEEDED(result)) result = outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    if (SUCCEEDED(result)) result = outputType->SetUINT32(MF_MT_AVG_BITRATE, 8'000'000);
    if (SUCCEEDED(result)) result = outputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    if (SUCCEEDED(result)) result = MFSetAttributeSize(outputType.Get(), MF_MT_FRAME_SIZE, width, height);
    if (SUCCEEDED(result)) {
        result = MFSetAttributeRatio(
            outputType.Get(), MF_MT_FRAME_RATE, frameRateNumerator, frameRateDenominator
        );
    }
    if (SUCCEEDED(result)) result = MFSetAttributeRatio(outputType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    if (SUCCEEDED(result)) result = writer->AddStream(outputType.Get(), &writerStream);

    Microsoft::WRL::ComPtr<IMFMediaType> inputType;
    if (SUCCEEDED(result)) result = MFCreateMediaType(&inputType);
    if (SUCCEEDED(result)) result = inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (SUCCEEDED(result)) result = inputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    if (SUCCEEDED(result)) result = inputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    if (SUCCEEDED(result)) result = MFSetAttributeSize(inputType.Get(), MF_MT_FRAME_SIZE, width, height);
    if (SUCCEEDED(result)) {
        result = inputType->SetUINT32(MF_MT_DEFAULT_STRIDE, width * 4u);
    }
    if (SUCCEEDED(result)) {
        result = MFSetAttributeRatio(
            inputType.Get(), MF_MT_FRAME_RATE, frameRateNumerator, frameRateDenominator
        );
    }
    if (SUCCEEDED(result)) result = MFSetAttributeRatio(inputType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    if (SUCCEEDED(result)) result = writer->SetInputMediaType(writerStream, inputType.Get(), nullptr);
    if (SUCCEEDED(result)) result = writer->BeginWriting();
    if (FAILED(result)) {
        writer.Reset();
        outError = videoHresultMessage("Starting MP4/H.264 recording", result);
        return false;
    }
    recordingEpochTimestamp100ns = -1;
    recordingLastSampleTime100ns = -1;
    outError.clear();
    return true;
}

bool VideoCaptureService::Implementation::finishWriter(std::string& outError) noexcept
{
    recordingActive.store(false, std::memory_order_release);
    if (!writer) {
        return outError.empty();
    }
    const std::string precedingError = outError;
    const HRESULT result = writer->Finalize();
    writer.Reset();
    recordingEpochTimestamp100ns = -1;
    recordingLastSampleTime100ns = -1;
    preparationState.store(
        VideoRecordingPreparationState::Idle, std::memory_order_release);
    if (FAILED(result)) {
        const std::string finalizeError =
            videoHresultMessage("Finalizing recorded video", result);
        outError = precedingError.empty()
            ? finalizeError
            : precedingError + "; " + finalizeError;
        return false;
    }
    if (!precedingError.empty()) {
        outError = precedingError + ". Finalized partial video kept at " +
            activeRecordingPartialPath.string();
        return false;
    }
    std::error_code renameError;
    std::filesystem::rename(
        activeRecordingPartialPath, activeRecordingFinalPath, renameError);
    if (renameError) {
        outError = "Publishing finalized video failed: " + renameError.message() +
            ". Partial video kept at " + activeRecordingPartialPath.string();
        return false;
    }
    outError.clear();
    return true;
}

} // namespace ovtr::win32
