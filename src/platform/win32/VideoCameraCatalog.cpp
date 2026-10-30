#include "platform/win32/VideoCameraCatalog.h"

#include <mfapi.h>
#include <mfidl.h>
#include <wrl/client.h>

#include <sstream>

namespace ovtr::win32 {
namespace {

std::string hresultMessage(const char* operation, const HRESULT result)
{
    std::ostringstream stream;
    stream << operation << " failed (HRESULT 0x" << std::hex
           << static_cast<unsigned long>(result) << ')';
    return stream.str();
}

std::wstring allocatedString(IMFActivate* activation, const GUID& key)
{
    wchar_t* text = nullptr;
    UINT32 length = 0;
    if (FAILED(activation->GetAllocatedString(key, &text, &length)) || !text) {
        return {};
    }
    std::wstring result(text, length);
    CoTaskMemFree(text);
    return result;
}

} // namespace

VideoCameraCatalogResult enumerateVideoCameras()
{
    VideoCameraCatalogResult output;
    Microsoft::WRL::ComPtr<IMFAttributes> attributes;
    HRESULT result = MFCreateAttributes(&attributes, 1);
    if (SUCCEEDED(result)) {
        result = attributes->SetGUID(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
        );
    }
    if (FAILED(result)) {
        output.error = hresultMessage("Preparing camera enumeration", result);
        return output;
    }

    IMFActivate** activations = nullptr;
    UINT32 count = 0;
    result = MFEnumDeviceSources(attributes.Get(), &activations, &count);
    if (FAILED(result)) {
        output.error = hresultMessage("Enumerating cameras", result);
        return output;
    }
    for (UINT32 index = 0; index < count; ++index) {
        VideoCameraInfo camera;
        camera.displayName = allocatedString(activations[index], MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME);
        camera.symbolicLink = allocatedString(
            activations[index], MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK
        );
        if (!camera.displayName.empty() && !camera.symbolicLink.empty()) {
            output.cameras.push_back(std::move(camera));
        }
        activations[index]->Release();
    }
    CoTaskMemFree(activations);
    output.success = true;
    return output;
}

} // namespace ovtr::win32
