#include "TestCases.h"
#include "TestSupport.h"

#include "export/FbxAsciiExporter.h"
#include "math/QuaternionUtils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace ovtr::test {
namespace {

constexpr double kTestPi = 3.14159265358979323846;
constexpr double kTestDegreesToRadians = kTestPi / 180.0;

std::array<double, 9> multiplyMatrix3x3(const std::array<double, 9>& left, const std::array<double, 9>& right)
{
    std::array<double, 9> result{};
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            result[static_cast<std::size_t>(row * 3 + column)] =
                left[static_cast<std::size_t>(row * 3 + 0)] * right[static_cast<std::size_t>(0 * 3 + column)] +
                left[static_cast<std::size_t>(row * 3 + 1)] * right[static_cast<std::size_t>(1 * 3 + column)] +
                left[static_cast<std::size_t>(row * 3 + 2)] * right[static_cast<std::size_t>(2 * 3 + column)];
        }
    }
    return result;
}

std::array<double, 9> matrixFromEulerXyzDegrees(const std::array<double, 3>& eulerDegrees)
{
    const double x = eulerDegrees[0] * kTestDegreesToRadians;
    const double y = eulerDegrees[1] * kTestDegreesToRadians;
    const double z = eulerDegrees[2] * kTestDegreesToRadians;
    const double cx = std::cos(x);
    const double sx = std::sin(x);
    const double cy = std::cos(y);
    const double sy = std::sin(y);
    const double cz = std::cos(z);
    const double sz = std::sin(z);

    const std::array<double, 9> rotateX{
        1.0, 0.0, 0.0,
        0.0, cx, -sx,
        0.0, sx, cx,
    };
    const std::array<double, 9> rotateY{
        cy, 0.0, sy,
        0.0, 1.0, 0.0,
        -sy, 0.0, cy,
    };
    const std::array<double, 9> rotateZ{
        cz, -sz, 0.0,
        sz, cz, 0.0,
        0.0, 0.0, 1.0,
    };

    return multiplyMatrix3x3(multiplyMatrix3x3(rotateZ, rotateY), rotateX);
}

std::array<double, 9> matrixFromQuaternion(const std::array<float, 4>& quaternion)
{
    const std::array<float, 4> q = ovtr::normalizeQuaternion(quaternion);
    const double x = q[0];
    const double y = q[1];
    const double z = q[2];
    const double w = q[3];

    return {
        1.0 - 2.0 * (y * y + z * z), 2.0 * (x * y - z * w), 2.0 * (x * z + y * w),
        2.0 * (x * y + z * w), 1.0 - 2.0 * (x * x + z * z), 2.0 * (y * z - x * w),
        2.0 * (x * z - y * w), 2.0 * (y * z + x * w), 1.0 - 2.0 * (x * x + y * y),
    };
}

double maxMatrixDelta(const std::array<double, 9>& left, const std::array<double, 9>& right)
{
    double delta = 0.0;
    for (std::size_t i = 0; i < left.size(); ++i) {
        delta = std::max(delta, std::fabs(left[i] - right[i]));
    }
    return delta;
}

} // namespace

void testFbxQuaternionToEuler()
{
    const auto identity = ovtr::quaternionToEulerXyzDegrees({0.0f, 0.0f, 0.0f, 1.0f});
    require(std::fabs(identity[0]) < 0.001, "identity quaternion roll mismatch");
    require(std::fabs(identity[1]) < 0.001, "identity quaternion pitch mismatch");
    require(std::fabs(identity[2]) < 0.001, "identity quaternion yaw mismatch");

    const float halfSqrt = std::sqrt(0.5f);
    const auto x90 = ovtr::quaternionToEulerXyzDegrees({halfSqrt, 0.0f, 0.0f, halfSqrt});
    const auto y90 = ovtr::quaternionToEulerXyzDegrees({0.0f, halfSqrt, 0.0f, halfSqrt});
    const auto z90 = ovtr::quaternionToEulerXyzDegrees({0.0f, 0.0f, halfSqrt, halfSqrt});
    require(std::fabs(x90[0] - 90.0) < 0.01, "x 90 quaternion euler mismatch");
    require(std::fabs(y90[1] - 90.0) < 0.01, "y 90 quaternion euler mismatch");
    require(std::fabs(z90[2] - 90.0) < 0.01, "z 90 quaternion euler mismatch");

    const auto mixedQuaternion = ovtr::normalizeQuaternion({0.182574f, -0.365148f, 0.547723f, 0.730297f});
    const auto mixedEuler = ovtr::quaternionToEulerXyzDegrees(mixedQuaternion);
    const auto sourceMatrix = matrixFromQuaternion(mixedQuaternion);
    const auto reconstructedMatrix = matrixFromEulerXyzDegrees(mixedEuler);
    require(
        maxMatrixDelta(sourceMatrix, reconstructedMatrix) < 0.00001,
        "mixed-axis quaternion should round-trip through FBX XYZ euler"
    );
}

} // namespace ovtr::test
