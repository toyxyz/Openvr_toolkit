#pragma once

#include "vr/IVRProvider.h"

#include <array>

namespace ovtr::win32 {

struct AppWindowState;

std::array<float, 3> vmcLocalPositionToAppOffset(const std::array<float, 3>& value) noexcept;
std::array<float, 4> vmcLocalRotationToAppRotation(const std::array<float, 4>& value) noexcept;
void appendVmcFingerPoses(AppWindowState& state, ovtr::PosePollResult& poses);

} // namespace ovtr::win32
