#include "TestCases.h"
#include "TestSupport.h"

#include "platform/win32/AppViewportState.h"
#include "platform/win32/ViewportCamera.h"
#include "platform/win32/ViewportMath.h"

#include <cmath>

namespace {

bool nearFloat(const float actual, const float expected, const float epsilon = 0.00001f)
{
    return std::fabs(actual - expected) < epsilon;
}

bool nearVec3(const ovtr::win32::Vec3 actual, const ovtr::win32::Vec3 expected)
{
    return nearFloat(actual.x, expected.x) &&
        nearFloat(actual.y, expected.y) &&
        nearFloat(actual.z, expected.z);
}

} // namespace

namespace ovtr::test {

void testWin32ViewportMath()
{
    require(ovtr::win32::clampFloat(12.0f, -2.0f, 10.0f) == 10.0f, "viewport clamp max");
    require(ovtr::win32::clampFloat(-3.0f, -2.0f, 10.0f) == -2.0f, "viewport clamp min");
    require(ovtr::win32::positiveCameraDistance(0.01f, 0.08f) == 0.08f, "camera distance min clamp");
    require(ovtr::win32::positiveCameraDistance(1.5f, 0.08f) == 1.5f, "camera distance keeps positive");

    require(
        nearVec3(ovtr::win32::normalizeVec3({3.0f, 4.0f, 0.0f}), {0.6f, 0.8f, 0.0f}),
        "normalize vec3"
    );
    require(
        nearVec3(ovtr::win32::rotateByInverseViewRotation(0.0f, 0.0f, {0.0f, 0.0f, -1.0f}), {0.0f, 0.0f, -1.0f}),
        "inverse view rotation identity"
    );
    require(
        nearVec3(ovtr::win32::rotateByInverseViewRotation(90.0f, 0.0f, {1.0f, 0.0f, 0.0f}), {0.0f, 0.0f, 1.0f}),
        "inverse view rotation yaw"
    );

    const ovtr::win32::CameraView view{
        0.0f,
        0.0f,
        10.0f,
        {0.0f, 0.0f, 0.0f},
    };
    require(
        nearVec3(ovtr::win32::screenSpacePanOffset(view, 10, -5, 0.08f), {-0.18f, -0.09f, 0.0f}),
        "screen-space pan offset"
    );
    require(
        nearVec3(ovtr::win32::cameraDollyOffset(view, 0.45f), {0.0f, 0.0f, -0.45f}),
        "camera dolly offset"
    );
    require(
        nearFloat(ovtr::win32::cameraDepthForWorldPoint(view, {0.0f, 0.0f, 0.0f}, 0.08f), 10.0f),
        "camera depth for origin"
    );
    require(
        nearFloat(ovtr::win32::cameraDepthForWorldPoint(view, {0.0f, 0.0f, 20.0f}, 0.08f), 0.001f),
        "camera depth minimum"
    );

    const float outline = ovtr::win32::outlineExpansionForDepth(10.0f, 1000, 48.0f, 2.6f, 1.0f);
    require(outline > 0.023f && outline < 0.024f, "outline expansion scales with viewport height");
    require(
        ovtr::win32::outlineExpansionForDepth(10.0f, 0, 48.0f, 2.6f, 1.0f) == 0.0f,
        "outline expansion rejects empty viewport"
    );

    ovtr::win32::AppViewportState viewportState;
    viewportState.cameraYawDegrees = 0.0f;
    viewportState.cameraPitchDegrees = 0.0f;
    viewportState.cameraDistance = 10.0f;
    require(
        nearFloat(ovtr::win32::cameraDepthForWorldPoint(viewportState, {0.0f, 0.0f, 0.0f}), 10.0f),
        "viewport state camera depth"
    );
    ovtr::win32::applyScreenSpacePan(viewportState, 10, -5);
    require(
        nearVec3(
            {viewportState.cameraPanX, viewportState.cameraPanY, viewportState.cameraPanZ},
            {-0.18f, -0.09f, 0.0f}
        ),
        "viewport state screen-space pan"
    );
    ovtr::win32::applyCameraDolly(viewportState, 0.45f);
    require(
        nearVec3(
            {viewportState.cameraPanX, viewportState.cameraPanY, viewportState.cameraPanZ},
            {-0.18f, -0.09f, -0.45f}
        ),
        "viewport state camera dolly"
    );

    float projection[16]{};
    ovtr::win32::perspectiveMatrix(48.0f, 2.0f, 0.05f, 100.0f, projection);
    require(nearFloat(projection[0], 1.12302f, 0.0001f), "perspective matrix x scale");
    require(nearFloat(projection[5], 2.24604f, 0.0001f), "perspective matrix y scale");
    require(nearFloat(projection[10], -1.001f, 0.0001f), "perspective matrix depth scale");
    require(projection[11] == -1.0f, "perspective matrix homogeneous term");
    require(nearFloat(projection[14], -0.10005f, 0.0001f), "perspective matrix depth offset");
}

} // namespace ovtr::test
