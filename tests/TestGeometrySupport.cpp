#include "TestSupport.h"

#include <cmath>

namespace ovtr::test {

ovtr::RenderModelGeometry makeTriangleGeometry()
{
    ovtr::RenderModelGeometry geometry;
    ovtr::RenderModelVertex v0;
    v0.position = {0.0f, 0.0f, 0.0f};
    v0.normal = {0.0f, 0.0f, 1.0f};
    v0.texCoord = {0.0f, 0.0f};

    ovtr::RenderModelVertex v1;
    v1.position = {1.0f, 0.0f, 0.0f};
    v1.normal = {0.0f, 0.0f, 1.0f};
    v1.texCoord = {1.0f, 0.0f};

    ovtr::RenderModelVertex v2;
    v2.position = {0.0f, 1.0f, 0.0f};
    v2.normal = {0.0f, 0.0f, 1.0f};
    v2.texCoord = {0.0f, 1.0f};

    geometry.available = true;
    geometry.vertices = {v0, v1, v2};
    geometry.indices = {0, 1, 2};
    return geometry;
}

std::array<float, 4> axisAngleQuaternionZ(const double degrees)
{
    constexpr double degreesToRadians = 3.14159265358979323846 / 180.0;
    const double halfAngle = degrees * degreesToRadians * 0.5;
    return {0.0f, 0.0f, static_cast<float>(std::sin(halfAngle)), static_cast<float>(std::cos(halfAngle))};
}

std::array<float, 4> axisAngleQuaternionY(const double degrees)
{
    constexpr double degreesToRadians = 3.14159265358979323846 / 180.0;
    const double halfAngle = degrees * degreesToRadians * 0.5;
    return {0.0f, static_cast<float>(std::sin(halfAngle)), 0.0f, static_cast<float>(std::cos(halfAngle))};
}

} // namespace ovtr::test
