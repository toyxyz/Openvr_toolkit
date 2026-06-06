#include "platform/win32/SkeletonGltfAnimationWriter.h"

#include "platform/win32/SkeletonGltfHierarchy.h"

#include <cstddef>
#include <ostream>

namespace ovtr::win32 {
namespace {

constexpr int kFirstJointNode = 0;

int jointNodeIndex(const int joint) noexcept
{
    int index = kFirstJointNode;
    for (int current = 0; current < joint; ++current) {
        if (isSkeletonGltfExportedJoint(current)) {
            ++index;
        }
    }
    return index;
}

void writeRotationSamplers(
    std::ostream& out,
    const int timeAccessor,
    const std::array<int, kProfileSkeletonJointCount>& rotationAccessors
) {
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        if (!isSkeletonGltfRotationAnimatedJoint(joint)) {
            continue;
        }
        out << ",\n        { \"input\": " << timeAccessor << ", \"output\": "
            << rotationAccessors[static_cast<std::size_t>(joint)] << ", \"interpolation\": \"LINEAR\" }";
    }
}

void writeTranslationSamplers(
    std::ostream& out,
    const int timeAccessor,
    const std::array<int, kProfileSkeletonJointCount>& translationAccessors
) {
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        if (!isSkeletonGltfTranslationAnimatedJoint(joint)) {
            continue;
        }
        out << ",\n        { \"input\": " << timeAccessor << ", \"output\": "
            << translationAccessors[static_cast<std::size_t>(joint)] << ", \"interpolation\": \"LINEAR\" }";
    }
}

void writeRotationChannels(std::ostream& out, int& sampler)
{
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        if (!isSkeletonGltfRotationAnimatedJoint(joint)) {
            continue;
        }
        out << ",\n        { \"sampler\": " << sampler << ", \"target\": { \"node\": "
            << jointNodeIndex(joint) << ", \"path\": \"rotation\" } }";
        ++sampler;
    }
}

void writeTranslationChannels(std::ostream& out, int& sampler)
{
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        if (!isSkeletonGltfTranslationAnimatedJoint(joint)) {
            continue;
        }
        out << ",\n        { \"sampler\": " << sampler << ", \"target\": { \"node\": "
            << jointNodeIndex(joint) << ", \"path\": \"translation\" } }";
        ++sampler;
    }
}

} // namespace

bool isSkeletonGltfRotationAnimatedJoint(const int joint) noexcept
{
    return isSkeletonGltfSkinJoint(joint) &&
        joint != kProfileJointLeftToeBase &&
        joint != kProfileJointRightToeBase;
}

bool isSkeletonGltfTranslationAnimatedJoint(const int joint) noexcept
{
    (void)joint;
    return false;
}

void writeSkeletonGltfAnimation(
    std::ostream& out,
    const int timeAccessor,
    const int rootTranslationAccessor,
    const std::array<int, kProfileSkeletonJointCount>& rotationAccessors,
    const std::array<int, kProfileSkeletonJointCount>& translationAccessors
) {
    out << "  \"animations\": [\n    {\n";
    out << "      \"name\": \"Skeleton Motion\",\n      \"samplers\": [\n";
    out << "        { \"input\": " << timeAccessor << ", \"output\": "
        << rootTranslationAccessor << ", \"interpolation\": \"LINEAR\" }";
    writeRotationSamplers(out, timeAccessor, rotationAccessors);
    writeTranslationSamplers(out, timeAccessor, translationAccessors);
    out << "\n      ],\n      \"channels\": [\n";
    out << "        { \"sampler\": 0, \"target\": { \"node\": "
        << jointNodeIndex(kProfileJointSpine) << ", \"path\": \"translation\" } }";
    int sampler = 1;
    writeRotationChannels(out, sampler);
    writeTranslationChannels(out, sampler);
    out << "\n      ]\n    }\n  ],\n";
}

} // namespace ovtr::win32
