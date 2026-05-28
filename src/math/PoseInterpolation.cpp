#include "math/PoseInterpolation.h"

#include "math/QuaternionUtils.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace ovtr {

double clamp01(const double value)
{
    return std::clamp(value, 0.0, 1.0);
}

std::array<float, 3> lerpVec3(
    const std::array<float, 3>& from,
    const std::array<float, 3>& to,
    const double factor
)
{
    return {
        static_cast<float>(from[0] + (to[0] - from[0]) * factor),
        static_cast<float>(from[1] + (to[1] - from[1]) * factor),
        static_cast<float>(from[2] + (to[2] - from[2]) * factor),
    };
}

std::array<double, 3> lerpVec3(
    const std::array<double, 3>& from,
    const std::array<double, 3>& to,
    const double factor
)
{
    return {
        from[0] + (to[0] - from[0]) * factor,
        from[1] + (to[1] - from[1]) * factor,
        from[2] + (to[2] - from[2]) * factor,
    };
}

double quaternionDot(const std::array<float, 4>& left, const std::array<float, 4>& right)
{
    return static_cast<double>(left[0]) * right[0] +
           static_cast<double>(left[1]) * right[1] +
           static_cast<double>(left[2]) * right[2] +
           static_cast<double>(left[3]) * right[3];
}

std::array<float, 4> slerpQuaternion(
    const std::array<float, 4>& from,
    const std::array<float, 4>& to,
    const double factor
)
{
    std::array<float, 4> start = normalizeQuaternion(from);
    std::array<float, 4> end = normalizeQuaternion(to);
    double dot = quaternionDot(start, end);

    if (dot < 0.0) {
        for (float& component : end) {
            component = -component;
        }
        dot = -dot;
    }

    if (dot > 0.9995) {
        std::array<float, 4> result{};
        for (int component = 0; component < 4; ++component) {
            result[static_cast<std::size_t>(component)] =
                static_cast<float>(
                    start[static_cast<std::size_t>(component)] +
                    (end[static_cast<std::size_t>(component)] -
                        start[static_cast<std::size_t>(component)]) * factor
                );
        }
        return normalizeQuaternion(result);
    }

    dot = std::clamp(dot, -1.0, 1.0);
    const double theta = std::acos(dot);
    const double sinTheta = std::sin(theta);
    const double startWeight = std::sin((1.0 - factor) * theta) / sinTheta;
    const double endWeight = std::sin(factor * theta) / sinTheta;

    return normalizeQuaternion({
        static_cast<float>(start[0] * startWeight + end[0] * endWeight),
        static_cast<float>(start[1] * startWeight + end[1] * endWeight),
        static_cast<float>(start[2] * startWeight + end[2] * endWeight),
        static_cast<float>(start[3] * startWeight + end[3] * endWeight),
    });
}

} // namespace ovtr
