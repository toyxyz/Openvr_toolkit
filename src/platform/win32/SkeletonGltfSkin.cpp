#include "platform/win32/SkeletonGltfSkin.h"

#include "platform/win32/SkeletonGltfHierarchy.h"

#include <array>
#include <cstddef>

namespace ovtr::win32 {
namespace {

using Matrix4 = std::array<float, 16>;

Matrix4 identityMatrix() noexcept
{
    return Matrix4{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
}

float matrixAt(const Matrix4& m, const int row, const int col) noexcept
{
    return m[static_cast<std::size_t>(col * 4 + row)];
}

void setMatrixAt(Matrix4& m, const int row, const int col, const float value) noexcept
{
    m[static_cast<std::size_t>(col * 4 + row)] = value;
}

Matrix4 multiplyMatrix(const Matrix4& a, const Matrix4& b) noexcept
{
    Matrix4 out{};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            float value = 0.0f;
            for (int k = 0; k < 4; ++k) { value += matrixAt(a, row, k) * matrixAt(b, k, col); }
            setMatrixAt(out, row, col, value);
        }
    }
    return out;
}

Matrix4 localMatrix(const SkeletonBonePose& bone) noexcept
{
    Matrix4 m = identityMatrix();
    const auto& q = bone.localRotation;
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    setMatrixAt(m, 0, 0, 1.0f - 2.0f * y * y - 2.0f * z * z);
    setMatrixAt(m, 0, 1, 2.0f * x * y - 2.0f * z * w);
    setMatrixAt(m, 0, 2, 2.0f * x * z + 2.0f * y * w);
    setMatrixAt(m, 1, 0, 2.0f * x * y + 2.0f * z * w);
    setMatrixAt(m, 1, 1, 1.0f - 2.0f * x * x - 2.0f * z * z);
    setMatrixAt(m, 1, 2, 2.0f * y * z - 2.0f * x * w);
    setMatrixAt(m, 2, 0, 2.0f * x * z - 2.0f * y * w);
    setMatrixAt(m, 2, 1, 2.0f * y * z + 2.0f * x * w);
    setMatrixAt(m, 2, 2, 1.0f - 2.0f * x * x - 2.0f * y * y);
    setMatrixAt(m, 0, 3, bone.localTranslationMeters.x);
    setMatrixAt(m, 1, 3, bone.localTranslationMeters.y);
    setMatrixAt(m, 2, 3, bone.localTranslationMeters.z);
    return m;
}

Matrix4 inverseRigidMatrix(const Matrix4& m) noexcept
{
    Matrix4 inv = identityMatrix();
    const std::array<float, 3> t{matrixAt(m, 0, 3), matrixAt(m, 1, 3), matrixAt(m, 2, 3)};
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) { setMatrixAt(inv, row, col, matrixAt(m, col, row)); }
    }
    for (int row = 0; row < 3; ++row) {
        setMatrixAt(inv, row, 3, -(matrixAt(inv, row, 0) * t[0] + matrixAt(inv, row, 1) * t[1] + matrixAt(inv, row, 2) * t[2]));
    }
    return inv;
}

} // namespace

std::vector<float> makeSkeletonInverseBindMatrices(const ProfileSkeletonJoints& rest, const SkeletonPose& bindPose)
{
    std::array<Matrix4, kProfileSkeletonJointCount> global{};
    std::vector<float> values;
    values.reserve(kProfileSkeletonJointCount * 16U);
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        const Matrix4 local = localMatrix(bindPose.bones[static_cast<std::size_t>(joint)]);
        const int parent = skeletonGltfParentIndex(rest, joint);
        global[static_cast<std::size_t>(joint)] = parent >= 0
            ? multiplyMatrix(global[static_cast<std::size_t>(parent)], local)
            : local;
        const Matrix4 inverse = inverseRigidMatrix(global[static_cast<std::size_t>(joint)]);
        values.insert(values.end(), inverse.begin(), inverse.end());
    }
    return values;
}

} // namespace ovtr::win32
