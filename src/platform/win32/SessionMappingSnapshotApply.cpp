#include "platform/win32/SessionMappingSnapshot.h"

#include "platform/win32/AppLoadedSessionState.h"

#include <utility>

namespace ovtr::win32 {
namespace {

std::uint32_t nextActorIdAfter(const std::vector<MappingActor>& actors) noexcept
{
    std::uint32_t nextId = 1;
    for (const MappingActor& actor : actors) {
        if (actor.id >= nextId) {
            nextId = actor.id + 1;
        }
    }
    return nextId;
}

float selectedArmSoftIk(const SessionMappingSnapshot& snapshot) noexcept
{
    for (const MappingActor& actor : snapshot.actors) {
        if (actor.id == snapshot.selectedActorId) {
            return actor.calibration.armSoftIkStrength;
        }
    }
    return snapshot.actors.empty()
        ? kDefaultMappingArmSoftIkStrength
        : snapshot.actors.front().calibration.armSoftIkStrength;
}

float selectedLegSoftIk(const SessionMappingSnapshot& snapshot) noexcept
{
    for (const MappingActor& actor : snapshot.actors) {
        if (actor.id == snapshot.selectedActorId) {
            return actor.calibration.legSoftIkStrength;
        }
    }
    return snapshot.actors.empty()
        ? kDefaultMappingLegSoftIkStrength
        : snapshot.actors.front().calibration.legSoftIkStrength;
}

bool selectedPinHand(const SessionMappingSnapshot& snapshot) noexcept
{
    for (const MappingActor& actor : snapshot.actors) {
        if (actor.id == snapshot.selectedActorId) {
            return actor.calibration.pinHandTargets;
        }
    }
    return snapshot.actors.empty() ? true : snapshot.actors.front().calibration.pinHandTargets;
}

bool selectedPinFoot(const SessionMappingSnapshot& snapshot) noexcept
{
    for (const MappingActor& actor : snapshot.actors) {
        if (actor.id == snapshot.selectedActorId) {
            return actor.calibration.pinFootTargets;
        }
    }
    return snapshot.actors.empty() ? true : snapshot.actors.front().calibration.pinFootTargets;
}

std::wstring activeActorName(const AppProfileState& profileState)
{
    for (const MappingActor& actor : profileState.mappingActors) {
        if (actor.id == profileState.selectedMappingActorId) {
            return effectiveMappingActorName(actor);
        }
    }
    return profileState.mappingActors.empty()
        ? profileState.profile.name
        : effectiveMappingActorName(profileState.mappingActors.front());
}

} // namespace

void backupLiveMappingForLoadedSession(AppLoadedSessionState& loadedState, const AppProfileState& profileState)
{
    loadedState.loadedSessionLiveProfile = profileState.profile;
    loadedState.loadedSessionLiveMappingActorName = profileState.mappingActorName;
    loadedState.loadedSessionLiveMappingSkeletonColor = profileState.mappingSkeletonColor;
    loadedState.loadedSessionLiveMappingSkeletonColorCustomized = profileState.mappingSkeletonColorCustomized;
    loadedState.loadedSessionLiveMappingDeviceRuntimeIndices = profileState.mappingDeviceRuntimeIndices;
    loadedState.loadedSessionLiveMappingFingerRuntimeIndices = profileState.mappingFingerRuntimeIndices;
    loadedState.loadedSessionLiveMappingActors = profileState.mappingActors;
    loadedState.loadedSessionLiveNextMappingActorId = profileState.nextMappingActorId;
    loadedState.loadedSessionLiveSelectedMappingActorId = profileState.selectedMappingActorId;
    loadedState.loadedSessionLiveArmSoftIkStrength = profileState.mappingArmSoftIkStrength;
    loadedState.loadedSessionLiveLegSoftIkStrength = profileState.mappingLegSoftIkStrength;
    loadedState.loadedSessionLivePinHandTargets = profileState.mappingPinHandTargets;
    loadedState.loadedSessionLivePinFootTargets = profileState.mappingPinFootTargets;
    loadedState.loadedSessionLiveMappingBackupValid = true;
}

void restoreLiveMappingAfterLoadedSession(AppLoadedSessionState& loadedState, AppProfileState& profileState)
{
    if (!loadedState.loadedSessionLiveMappingBackupValid) {
        return;
    }
    profileState.profile = std::move(loadedState.loadedSessionLiveProfile);
    profileState.mappingActorName = std::move(loadedState.loadedSessionLiveMappingActorName);
    profileState.mappingSkeletonColor = loadedState.loadedSessionLiveMappingSkeletonColor;
    profileState.mappingSkeletonColorCustomized = loadedState.loadedSessionLiveMappingSkeletonColorCustomized;
    profileState.mappingDeviceRuntimeIndices = loadedState.loadedSessionLiveMappingDeviceRuntimeIndices;
    profileState.mappingFingerRuntimeIndices = loadedState.loadedSessionLiveMappingFingerRuntimeIndices;
    profileState.mappingActors = std::move(loadedState.loadedSessionLiveMappingActors);
    profileState.nextMappingActorId = loadedState.loadedSessionLiveNextMappingActorId;
    profileState.selectedMappingActorId = loadedState.loadedSessionLiveSelectedMappingActorId;
    profileState.mappingArmSoftIkStrength = loadedState.loadedSessionLiveArmSoftIkStrength;
    profileState.mappingLegSoftIkStrength = loadedState.loadedSessionLiveLegSoftIkStrength;
    profileState.mappingPinHandTargets = loadedState.loadedSessionLivePinHandTargets;
    profileState.mappingPinFootTargets = loadedState.loadedSessionLivePinFootTargets;
    loadedState.loadedSessionLiveMappingBackupValid = false;
}

void applySessionMappingSnapshotToLiveMapping(AppProfileState& profileState, SessionMappingSnapshot snapshot)
{
    const float armSoftIk = selectedArmSoftIk(snapshot);
    const float legSoftIk = selectedLegSoftIk(snapshot);
    const bool pinHand = selectedPinHand(snapshot);
    const bool pinFoot = selectedPinFoot(snapshot);
    profileState.profile = std::move(snapshot.profile);
    profileState.mappingSkeletonColor = snapshot.mappingSkeletonColor;
    profileState.mappingSkeletonColorCustomized = true;
    profileState.mappingDeviceRuntimeIndices = snapshot.mappingDeviceRuntimeIndices;
    profileState.mappingFingerRuntimeIndices = snapshot.mappingFingerRuntimeIndices;
    profileState.mappingActors = std::move(snapshot.actors);
    profileState.selectedMappingActorId = snapshot.selectedActorId;
    profileState.mappingActorName = activeActorName(profileState);
    profileState.nextMappingActorId = nextActorIdAfter(profileState.mappingActors);
    profileState.mappingArmSoftIkStrength = armSoftIk;
    profileState.mappingLegSoftIkStrength = legSoftIk;
    profileState.mappingPinHandTargets = pinHand;
    profileState.mappingPinFootTargets = pinFoot;
}

} // namespace ovtr::win32
