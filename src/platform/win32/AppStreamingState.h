#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/ConfigTypes.h"
#include "platform/win32/RealtimePoseSmoothing.h"
#include "platform/win32/StreamingOutputTarget.h"
#include "platform/win32/VmcLegRotationContinuity.h"
#include "platform/win32/VmcReceiver.h"
#include "platform/win32/VmcSender.h"

#include <mutex>
#include <string>

namespace ovtr::win32 {

struct AppStreamingState {
    mutable std::mutex realtimeSmoothingMutex;
    bool realtimeSmoothingEnabled = false;
    RealtimeSmoothingPreset realtimeSmoothingPreset = RealtimeSmoothingPreset::Normal;
    RealtimePoseSmoother realtimePoseSmoother;
    bool vmcReceiveEnabled = false;
    int vmcPort = 39540;
    VmcReceiver vmcReceiver;
    StreamingOutputTarget streamingOutputTarget = StreamingOutputTarget::None;
    bool streamingTargetDropdownOpen = false;
    std::string vmcSendHost = "192.168.0.2";
    int vmcSendPort = 39539;
    float vmcArmSpacingDegrees = 0.0f;
    float vmcLegSpacingDegrees = 0.0f;
    bool vmcSendErrorLogged = false;
    HWND vmcSendHostEditWindow = nullptr;
    WNDPROC vmcSendHostEditOriginalProc = nullptr;
    HWND vmcSendPortEditWindow = nullptr;
    WNDPROC vmcSendPortEditOriginalProc = nullptr;
    HWND vmcArmSpacingEditWindow = nullptr;
    WNDPROC vmcArmSpacingEditOriginalProc = nullptr;
    HWND vmcLegSpacingEditWindow = nullptr;
    WNDPROC vmcLegSpacingEditOriginalProc = nullptr;
    VmcLegRotationContinuity vmcLegRotationContinuity;
    VmcSender vmcSender;
};

} // namespace ovtr::win32
