#pragma once

#include "platform/win32/AppProfileState.h"
#include "platform/win32/MappingActorLayout.h"

#include <cstdint>
#include <string>
#include <utility>

namespace ovtr::win32 {

inline bool mappingActorNameExists(const AppProfileState& state, const std::wstring& name)
{
    for (const MappingActor& actor : state.mappingActors) {
        if (effectiveMappingActorName(actor) == name) {
            return true;
        }
    }
    return false;
}

inline bool splitMappingActorNumericSuffix(
    const std::wstring& name,
    std::wstring& baseName,
    int& nextSuffix
) {
    const std::size_t separator = name.find_last_of(L'_');
    if (separator == std::wstring::npos || separator == 0 || separator + 1 >= name.size()) {
        return false;
    }

    int value = 0;
    for (std::size_t index = separator + 1; index < name.size(); ++index) {
        const wchar_t ch = name[index];
        if (ch < L'0' || ch > L'9') {
            return false;
        }
        value = (value * 10) + static_cast<int>(ch - L'0');
        if (value >= 100000) {
            return false;
        }
    }

    baseName = name.substr(0, separator);
    nextSuffix = value + 1;
    return true;
}

inline std::wstring uniqueMappingActorName(const AppProfileState& state, const std::wstring& requestedName)
{
    if (!mappingActorNameExists(state, requestedName)) {
        return requestedName;
    }

    std::wstring baseName = requestedName;
    int suffix = 0;
    splitMappingActorNumericSuffix(requestedName, baseName, suffix);
    for (; suffix < 100000; ++suffix) {
        const std::wstring candidate = baseName + L"_" + std::to_wstring(suffix);
        if (!mappingActorNameExists(state, candidate)) {
            return candidate;
        }
    }
    return requestedName + L"_" + std::to_wstring(state.nextMappingActorId);
}

inline std::uint32_t addMappingActorFromProfile(
    AppProfileState& state,
    const BodyProfile& profile,
    const int visibleActorRows
) {
    MappingActor actor;
    actor.id = state.nextMappingActorId++;
    actor.name = uniqueMappingActorName(state, L"actor_0");
    actor.profile = profile;
    actor.mappingDeviceRuntimeIndices = state.mappingDeviceRuntimeIndices;
    actor.mappingFingerRuntimeIndices = state.mappingFingerRuntimeIndices;
    actor.skeletonColor = state.mappingSkeletonColor;
    const std::uint32_t addedActorId = actor.id;
    state.mappingActors.push_back(std::move(actor));
    state.selectedMappingActorId = addedActorId;
    state.mappingActorName = state.mappingActors.back().name;
    state.mappingActorScrollOffset = maxMappingActorScrollOffset(state.mappingActors.size(), visibleActorRows);
    return addedActorId;
}

} // namespace ovtr::win32
