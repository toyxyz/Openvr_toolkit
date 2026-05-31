#include "platform/win32/ProfileModel.h"

#include <array>
#include <cmath>
#include <cwchar>
#include <cwctype>
#include <iomanip>
#include <sstream>

namespace ovtr::win32 {
namespace {

constexpr std::array<ProfileMeasurementDefinition, kProfileMeasurementCount> kProfileMeasurements{{
    {"floor_to_pelvis_center_height_cm", L"Floor to Pelvis Center Height"},
    {"pelvis_width_cm", L"Pelvis Width"},
    {"pelvis_center_to_neck_base_center_cm", L"Pelvis Center to Neck Base Center"},
    {"neck_base_center_to_head_top_cm", L"Neck Base Center to Head Top"},
    {"neck_length_cm", L"Neck Length"},
    {"shoulder_width_cm", L"Shoulder Width"},
    {"shoulder_to_elbow_cm", L"Shoulder to Elbow"},
    {"elbow_to_wrist_cm", L"Elbow to Wrist"},
    {"wrist_to_middle_finger_tip_cm", L"Wrist to Middle Finger Tip"},
    {"hip_to_knee_cm", L"Hip to Knee"},
    {"knee_to_ankle_cm", L"Knee to Ankle"},
    {"ankle_to_toe_tip_cm", L"Ankle to Toe Tip"},
    {"ankle_height_cm", L"Ankle Height"},
    {"toe_tip_height_cm", L"Toe Tip Height"},
}};

constexpr std::array<float, kProfileMeasurementCount> kDefaultMeasurements{{
    90.0f, 28.0f, 53.0f, 27.0f, 7.0f, 41.0f, 31.0f,
    26.0f, 19.0f, 42.0f, 40.0f, 25.0f, 8.0f, 2.0f
}};

enum MeasurementIndex {
    FloorToPelvis,
    PelvisWidth,
    PelvisToNeck,
    NeckToHeadTop,
};

bool hasOnlyTrailingWhitespace(const wchar_t* cursor) noexcept
{
    while (*cursor != L'\0') {
        if (std::iswspace(*cursor) == 0) {
            return false;
        }
        ++cursor;
    }
    return true;
}

} // namespace

BodyProfile::BodyProfile()
    : name(L"actor_01")
    , measurements(kDefaultMeasurements)
{
}

const std::array<ProfileMeasurementDefinition, kProfileMeasurementCount>& profileMeasurementDefinitions()
{
    return kProfileMeasurements;
}

float computedProfileHeightCm(const BodyProfile& profile) noexcept
{
    return profile.measurements[FloorToPelvis] +
        profile.measurements[PelvisToNeck] +
        profile.measurements[NeckToHeadTop];
}

std::wstring formatProfileNumber(const float value)
{
    if (std::isfinite(value) && std::fabs(value - std::round(value)) < 0.0005f) {
        return std::to_wstring(static_cast<int>(std::lround(value)));
    }

    std::wostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}

bool parseProfileNumberText(const std::wstring& text, float& outValue)
{
    std::wstring mutableText = text;
    wchar_t* begin = mutableText.data();
    wchar_t* end = nullptr;
    const float parsed = std::wcstof(begin, &end);
    if (end == begin || !std::isfinite(parsed) || parsed < 0.0f || !hasOnlyTrailingWhitespace(end)) {
        return false;
    }

    outValue = parsed;
    return true;
}

} // namespace ovtr::win32
