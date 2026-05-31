#include "platform/win32/ProfileEditModel.h"

#include "platform/win32/Win32String.h"

#include <cstddef>

namespace ovtr::win32 {

bool profileEditTargetIsValid(const ProfileEditTarget& target) noexcept
{
    return target.kind == ProfileEditKind::Name ||
        (target.kind == ProfileEditKind::Measurement &&
            target.measurementIndex >= 0 &&
            target.measurementIndex < kProfileMeasurementCount);
}

std::wstring profileTextForTarget(const BodyProfile& profile, const ProfileEditTarget& target)
{
    if (target.kind == ProfileEditKind::Name) {
        return profile.name;
    }
    if (target.kind == ProfileEditKind::Height) {
        return formatProfileNumber(computedProfileHeightCm(profile));
    }
    return formatProfileNumber(profile.measurements[static_cast<std::size_t>(target.measurementIndex)]);
}

void beginProfileEditSnapshot(AppProfileState& state)
{
    state.profileEditSnapshot = state.profile;
    state.profileEditSnapshotValid = true;
}

void clearProfileEditSnapshot(AppProfileState& state) noexcept
{
    state.profileEditSnapshotValid = false;
}

bool restoreProfileEditSnapshot(AppProfileState& state)
{
    if (!state.profileEditSnapshotValid) {
        return false;
    }
    state.profile = state.profileEditSnapshot;
    return true;
}

bool applyProfileEditLiveText(BodyProfile& profile, const ProfileEditTarget& target, const std::wstring& text)
{
    float value = 0.0f;
    if (!parseProfileNumberText(text, value)) {
        return false;
    }
    if (target.kind == ProfileEditKind::Measurement &&
        target.measurementIndex >= 0 &&
        target.measurementIndex < kProfileMeasurementCount) {
        profile.measurements[static_cast<std::size_t>(target.measurementIndex)] = value;
        return true;
    }
    return false;
}

bool applyProfileEditCommittedText(
    BodyProfile& profile,
    const ProfileEditTarget& target,
    const std::wstring& text,
    std::wstring& errorMessage
)
{
    if (target.kind == ProfileEditKind::Name) {
        const std::wstring trimmed = trimWide(text);
        if (trimmed.empty()) {
            errorMessage = L"Profile name cannot be empty.";
            return false;
        }
        profile.name = trimmed;
        return true;
    }

    if (applyProfileEditLiveText(profile, target, text)) {
        return true;
    }
    errorMessage = L"Enter a non-negative numeric value.";
    return false;
}

} // namespace ovtr::win32
