#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct VmcOscBonePose {
    std::string name;
    std::array<float, 3> position{};
    std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};
};

bool parseVmcOscPacket(
    const std::uint8_t* data,
    std::size_t size,
    std::vector<VmcOscBonePose>& outBones
);

} // namespace ovtr::win32
