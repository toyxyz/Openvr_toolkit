#pragma once

#include "platform/win32/AppProfileState.h"
#include "platform/win32/AppStreamingState.h"

#include <array>

namespace ovtr::win32 {

std::array<float, 3> appPositionToVmcPosition(Vec3 position) noexcept;
std::array<float, 4> appRotationToVmcRotation(const std::array<float, 4>& rotation) noexcept;

bool sendMappingActorVmcPose(
    AppStreamingState& streamingState,
    const MappingActor& actor,
    std::string& error
);
void stopVmcStreamingOutput(AppStreamingState& streamingState) noexcept;

} // namespace ovtr::win32
