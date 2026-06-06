#pragma once

#include "platform/win32/AppProfileState.h"
#include "platform/win32/MappingActorLayout.h"

#include <cstdint>
#include <utility>

namespace ovtr::win32 {

inline std::uint32_t addMappingActorFromProfile(
    AppProfileState& state,
    const BodyProfile& profile,
    const int visibleActorRows
) {
    MappingActor actor;
    actor.id = state.nextMappingActorId++;
    actor.profile = profile;
    actor.mappingDeviceRuntimeIndices = state.mappingDeviceRuntimeIndices;
    actor.skeletonColor = state.mappingSkeletonColor;
    const std::uint32_t addedActorId = actor.id;
    state.mappingActors.push_back(std::move(actor));
    state.selectedMappingActorId = addedActorId;
    state.mappingActorScrollOffset = maxMappingActorScrollOffset(state.mappingActors.size(), visibleActorRows);
    return addedActorId;
}

} // namespace ovtr::win32
