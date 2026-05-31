#pragma once

#include "platform/win32/AppProfileState.h"

#include <string>

namespace ovtr::win32 {

bool profileEditTargetIsValid(const ProfileEditTarget& target) noexcept;
std::wstring profileTextForTarget(const BodyProfile& profile, const ProfileEditTarget& target);
void beginProfileEditSnapshot(AppProfileState& state);
void clearProfileEditSnapshot(AppProfileState& state) noexcept;
bool restoreProfileEditSnapshot(AppProfileState& state);
bool applyProfileEditLiveText(BodyProfile& profile, const ProfileEditTarget& target, const std::wstring& text);
bool applyProfileEditCommittedText(
    BodyProfile& profile,
    const ProfileEditTarget& target,
    const std::wstring& text,
    std::wstring& errorMessage
);

} // namespace ovtr::win32
