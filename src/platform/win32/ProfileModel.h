#pragma once

#include <array>
#include <string>

namespace ovtr::win32 {

inline constexpr int kProfileMeasurementCount = 14;

struct ProfileMeasurementDefinition {
    const char* key = "";
    const wchar_t* label = L"";
};

struct BodyProfile {
    BodyProfile();

    std::wstring name;
    std::array<float, kProfileMeasurementCount> measurements{};
};

const std::array<ProfileMeasurementDefinition, kProfileMeasurementCount>& profileMeasurementDefinitions();
float computedProfileHeightCm(const BodyProfile& profile) noexcept;
std::wstring formatProfileNumber(float value);
bool parseProfileNumberText(const std::wstring& text, float& outValue);

} // namespace ovtr::win32
