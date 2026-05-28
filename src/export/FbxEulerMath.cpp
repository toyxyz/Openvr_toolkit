#include "export/FbxAsciiMath.h"

#include <cmath>

namespace ovtr {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = kPi * 2.0;
constexpr double kEulerHypotEpsilon = 0.0000375;

std::array<std::array<double, 3>, 2> matrix3ToEulerXyzCandidates(const FbxMatrix3& matrix)
{
    const double cy = std::hypot(matrix[0], matrix[3]);
    std::array<double, 3> primary{};
    std::array<double, 3> secondary{};

    if (cy > kEulerHypotEpsilon) {
        primary = {
            std::atan2(matrix[7], matrix[8]),
            std::atan2(-matrix[6], cy),
            std::atan2(matrix[3], matrix[0]),
        };
        secondary = {
            std::atan2(-matrix[7], -matrix[8]),
            std::atan2(-matrix[6], -cy),
            std::atan2(-matrix[3], -matrix[0]),
        };
    } else {
        primary = {
            std::atan2(-matrix[5], matrix[4]),
            std::atan2(-matrix[6], cy),
            0.0,
        };
        secondary = primary;
    }

    return {primary, secondary};
}

double squaredEulerDistance(const std::array<double, 3>& left, const std::array<double, 3>& right)
{
    double distance = 0.0;
    for (int axis = 0; axis < 3; ++axis) {
        const double delta = left[axis] - right[axis];
        distance += delta * delta;
    }
    return distance;
}

std::array<double, 3> nearestEulerPeriod(
    const std::array<double, 3>& euler,
    const std::array<double, 3>& reference
)
{
    std::array<double, 3> nearest = euler;
    for (int axis = 0; axis < 3; ++axis) {
        nearest[axis] += std::round((reference[axis] - nearest[axis]) / kTwoPi) * kTwoPi;
    }
    return nearest;
}

} // namespace

std::array<double, 3> fbxMatrix3ToEulerXyzRadians(const FbxMatrix3& matrix)
{
    return matrix3ToEulerXyzCandidates(matrix)[0];
}

std::array<double, 3> fbxCompatibleEulerXyzRadiansFromMatrix(
    const FbxMatrix3& matrix,
    const std::array<double, 3>& previous
)
{
    const auto baseCandidates = matrix3ToEulerXyzCandidates(matrix);
    std::array<double, 3> best = nearestEulerPeriod(baseCandidates[0], previous);
    double bestDistance = squaredEulerDistance(best, previous);

    for (const std::array<double, 3>& baseCandidate : baseCandidates) {
        const std::array<double, 3> centered = nearestEulerPeriod(baseCandidate, previous);
        for (int xOffset = -1; xOffset <= 1; ++xOffset) {
            for (int yOffset = -1; yOffset <= 1; ++yOffset) {
                for (int zOffset = -1; zOffset <= 1; ++zOffset) {
                    const std::array<double, 3> candidate{
                        centered[0] + static_cast<double>(xOffset) * kTwoPi,
                        centered[1] + static_cast<double>(yOffset) * kTwoPi,
                        centered[2] + static_cast<double>(zOffset) * kTwoPi,
                    };
                    const double distance = squaredEulerDistance(candidate, previous);
                    if (distance < bestDistance) {
                        best = candidate;
                        bestDistance = distance;
                    }
                }
            }
        }
    }

    return best;
}

void unwrapFbxEulerRadians(std::vector<std::array<double, 3>>& eulers)
{
    for (std::size_t index = 1; index < eulers.size(); ++index) {
        for (int axis = 0; axis < 3; ++axis) {
            const double diff = eulers[index][axis] - eulers[index - 1][axis];
            if (std::fabs(diff) > kPi) {
                eulers[index][axis] -= std::round(diff / kTwoPi) * kTwoPi;
            }
        }
    }
}

} // namespace ovtr
