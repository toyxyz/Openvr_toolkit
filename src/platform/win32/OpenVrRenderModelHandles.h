#pragma once

#include "vr/OpenVRRenderModelResources.h"

namespace ovtr::win32 {

#ifdef OVTR_HAS_OPENVR_SDK
using OpenVrTextureHandle = ::ovtr::OpenVRTextureHandle;
using OpenVrRenderModelHandle = ::ovtr::OpenVRRenderModelHandle;
#endif

} // namespace ovtr::win32
