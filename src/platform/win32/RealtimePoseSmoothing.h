#pragma once

#include "platform/win32/ConfigTypes.h"
#include "vr/IVRProvider.h"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace ovtr::win32 {

struct OneEuroPresetParameters {
    float minCutoffHz = 1.0f;
    float beta = 0.05f;
    float derivativeCutoffHz = 1.0f;
};

OneEuroPresetParameters oneEuroParametersForPreset(RealtimeSmoothingPreset preset) noexcept;
const char* realtimeSmoothingPresetLabel(RealtimeSmoothingPreset preset) noexcept;
const char* realtimeSmoothingPresetConfigValue(RealtimeSmoothingPreset preset) noexcept;
bool parseRealtimeSmoothingPresetConfigValue(const std::string& value, RealtimeSmoothingPreset& out);

class RealtimePoseSmoother {
public:
    void reset();
    void setPreset(RealtimeSmoothingPreset preset);
    void apply(ovtr::PosePollResult& poses);

private:
    struct PoseFilterState {
        std::array<float, 3> lastRaw{};
        std::array<float, 3> filtered{};
        std::array<float, 3> filteredDerivative{};
        std::uint64_t lastTimestampNs = 0;
        bool initialized = false;
    };

    RealtimeSmoothingPreset preset_ = RealtimeSmoothingPreset::Normal;
    std::unordered_map<std::uint32_t, PoseFilterState> filters_;
};

} // namespace ovtr::win32
