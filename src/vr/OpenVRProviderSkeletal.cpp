#include "vr/OpenVRProvider.h"

#include "data/SkeletalSyntheticPose.h"
#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"
#include "vr/OpenVRProviderDetails.h"
#include "vr/OpenVRProviderRuntime.h"

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <filesystem>
#include <string>
#include <vector>

namespace ovtr {
namespace {

#ifdef OVTR_HAS_OPENVR_SDK
constexpr const char* kSkeletalActionSetPath = "/actions/ovtr";
constexpr const char* kLeftSkeletalActionPath = "/actions/ovtr/in/skeletonleft";
constexpr const char* kRightSkeletalActionPath = "/actions/ovtr/in/skeletonright";
constexpr const char* kActionManifestFileName = "openvr_actions.json";

std::filesystem::path executableDirectoryPath()
{
    std::vector<wchar_t> buffer(MAX_PATH);
    while (true) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return std::filesystem::current_path();
        }
        if (length < buffer.size()) {
            return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
}

std::filesystem::path actionManifestPath()
{
    return executableDirectoryPath() / "config" / kActionManifestFileName;
}

std::string inputErrorMessage(const char* operation, const vr::EVRInputError error)
{
    return std::string(operation) + " failed: EVRInputError " + std::to_string(static_cast<int>(error));
}

void appendError(std::string& target, const std::string& message)
{
    if (!target.empty()) {
        target += "; ";
    }
    target += message;
}

bool validPoseActionData(const vr::InputPoseActionData_t& poseData) noexcept
{
    return poseData.bActive && poseData.pose.bDeviceIsConnected && poseData.pose.bPoseIsValid;
}

std::array<float, 4> quaternionFromBone(const vr::VRBoneTransform_t& bone)
{
    return {bone.orientation.x, bone.orientation.y, bone.orientation.z, bone.orientation.w};
}

std::array<float, 3> positionFromBone(const vr::VRBoneTransform_t& bone)
{
    return {bone.position.v[0], bone.position.v[1], bone.position.v[2]};
}

PoseSample makeSkeletalBonePose(
    const PoseSample& rootPose,
    const SkeletalHandSide side,
    const std::uint32_t boneIndex,
    const vr::VRBoneTransform_t& bone
)
{
    const std::array<float, 3> localPosition = positionFromBone(bone);
    const std::array<float, 3> rotatedPosition = rotatePositionByQuaternion(rootPose.rotation, localPosition);
    PoseSample pose;
    pose.deviceId = skeletalBoneRuntimeIndex(side, boneIndex);
    pose.runtimeIndex = pose.deviceId;
    pose.position = {
        rootPose.position[0] + rotatedPosition[0],
        rootPose.position[1] + rotatedPosition[1],
        rootPose.position[2] + rotatedPosition[2],
    };
    pose.rotation = normalizeQuaternion(multiplyQuaternion(rootPose.rotation, quaternionFromBone(bone)));
    pose.flags = PoseFlagDeviceConnected | PoseFlagPoseValid | PoseFlagRecordEnabled;
    return pose;
}
#endif

} // namespace

bool OpenVRProvider::initializeSkeletalInput()
{
#ifdef OVTR_HAS_OPENVR_SDK
    if (skeletalInput_.handlesResolved) {
        return skeletalInput_.available;
    }
    skeletalInput_.setupAttempted = true;

    vr::IVRInput* input = vr::VRInput();
    if (input == nullptr) {
        skeletalInput_.error = "VRInput interface is unavailable";
        return false;
    }

    const std::filesystem::path manifest = actionManifestPath();
    std::error_code existsError;
    if (!std::filesystem::exists(manifest, existsError)) {
        skeletalInput_.error = "missing SteamVR action manifest: " + manifest.string();
        return false;
    }

    vr::EVRInputError error = input->SetActionManifestPath(manifest.string().c_str());
    if (error != vr::VRInputError_None) {
        skeletalInput_.error = inputErrorMessage("SetActionManifestPath", error);
        return false;
    }

    error = input->GetActionSetHandle(kSkeletalActionSetPath, &skeletalInput_.actionSet);
    if (error != vr::VRInputError_None) {
        skeletalInput_.error = inputErrorMessage("GetActionSetHandle", error);
        return false;
    }
    error = input->GetActionHandle(kLeftSkeletalActionPath, &skeletalInput_.leftAction);
    if (error != vr::VRInputError_None) {
        skeletalInput_.error = inputErrorMessage("GetActionHandle left skeleton", error);
        return false;
    }
    error = input->GetActionHandle(kRightSkeletalActionPath, &skeletalInput_.rightAction);
    if (error != vr::VRInputError_None) {
        skeletalInput_.error = inputErrorMessage("GetActionHandle right skeleton", error);
        return false;
    }

    skeletalInput_.handlesResolved = true;
    skeletalInput_.available = true;
    skeletalInput_.error.clear();
    return skeletalInput_.available;
#else
    skeletalInput_.setupAttempted = true;
    skeletalInput_.available = false;
    return false;
#endif
}

void OpenVRProvider::appendSkeletalPoses(PosePollResult& outResult)
{
#ifdef OVTR_HAS_OPENVR_SDK
    if (!initializeSkeletalInput()) {
        return;
    }

    vr::IVRInput* input = vr::VRInput();
    if (input == nullptr) {
        skeletalInput_.error = "VRInput interface is unavailable while polling skeletal input";
        return;
    }

    vr::VRActiveActionSet_t actionSet{};
    actionSet.ulActionSet = skeletalInput_.actionSet;
    actionSet.ulRestrictedToDevice = vr::k_ulInvalidInputValueHandle;
    const vr::EVRInputError updateError = input->UpdateActionState(&actionSet, sizeof(actionSet), 1);
    if (updateError != vr::VRInputError_None) {
        skeletalInput_.error = inputErrorMessage("UpdateActionState", updateError);
        return;
    }
    std::string pollError;

    const auto ensureBoneCount = [&](const char* label, const std::uint64_t action, std::uint32_t& count) {
        if (action == 0) {
            return false;
        }
        if (count > 0) {
            return true;
        }
        const vr::EVRInputError countError = input->GetBoneCount(action, &count);
        if (countError != vr::VRInputError_None) {
            appendError(pollError, inputErrorMessage((std::string("GetBoneCount ") + label).c_str(), countError));
            return false;
        }
        return count > 0;
    };

    const auto appendHand = [&](const SkeletalHandSide side, const char* label, const std::uint64_t action, std::uint32_t& count) {
        if (!ensureBoneCount(label, action, count)) {
            return false;
        }
        const std::uint32_t boneCount = count < kSkeletalHandBoneCount ? count : kSkeletalHandBoneCount;
        if (boneCount == 0) {
            appendError(pollError, std::string(label) + " skeleton returned zero bones");
            return false;
        }

        const std::size_t poseCountBefore = outResult.poses.size();
        vr::InputSkeletalActionData_t actionData{};
        const vr::EVRInputError actionError =
            input->GetSkeletalActionData(action, &actionData, sizeof(actionData));
        if (actionError != vr::VRInputError_None) {
            appendError(pollError, inputErrorMessage((std::string("GetSkeletalActionData ") + label).c_str(), actionError));
            return false;
        }
        if (!actionData.bActive) {
            return false;
        }

        vr::InputPoseActionData_t poseData{};
        const vr::EVRInputError poseError = input->GetPoseActionDataRelativeToNow(
                action,
                vr::TrackingUniverseStanding,
                0.0f,
                &poseData,
                sizeof(poseData),
                vr::k_ulInvalidInputValueHandle
            );
        if (poseError != vr::VRInputError_None) {
            appendError(pollError, inputErrorMessage((std::string("GetPoseActionDataRelativeToNow ") + label).c_str(), poseError));
            return false;
        }
        if (!validPoseActionData(poseData)) {
            return false;
        }

        std::vector<vr::VRBoneTransform_t> bones(boneCount);
        const vr::EVRInputError boneError = input->GetSkeletalBoneData(
                action,
                vr::VRSkeletalTransformSpace_Model,
                vr::VRSkeletalMotionRange_WithoutController,
                bones.data(),
                boneCount
            );
        if (boneError != vr::VRInputError_None) {
            appendError(pollError, inputErrorMessage((std::string("GetSkeletalBoneData ") + label).c_str(), boneError));
            return false;
        }

        const PoseSample rootPose = openvr_provider_detail::makePoseSample(0, poseData.pose);
        for (std::uint32_t i = 0; i < boneCount; ++i) {
            if (!shouldRecordSkeletalBoneIndex(i)) {
                continue;
            }
            outResult.poses.push_back(makeSkeletalBonePose(rootPose, side, i, bones[i]));
        }
        return outResult.poses.size() > poseCountBefore;
    };

    const bool leftOk = appendHand(SkeletalHandSide::Left, "left skeleton", skeletalInput_.leftAction, skeletalInput_.leftBoneCount);
    const bool rightOk = appendHand(SkeletalHandSide::Right, "right skeleton", skeletalInput_.rightAction, skeletalInput_.rightBoneCount);
    skeletalInput_.leftAvailable = skeletalInput_.leftBoneCount > 0;
    skeletalInput_.rightAvailable = skeletalInput_.rightBoneCount > 0;
    skeletalInput_.available = skeletalInput_.leftAvailable || skeletalInput_.rightAvailable;
    skeletalInput_.error = (leftOk || rightOk || skeletalInput_.available) ? std::string{} : pollError;
#else
    (void)outResult;
#endif
}

} // namespace ovtr
