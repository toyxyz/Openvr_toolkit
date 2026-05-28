#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportGlMatrixScope.h"
#include "platform/win32/ViewportGlStateScope.h"

namespace ovtr::win32 {

void drawDeviceMarker3D(
    const ovtr::PoseSample& pose,
    const ovtr::DeviceClass deviceClass,
    const bool drawBody,
    const bool selected
)
{
    const float x = pose.position[0];
    const float y = pose.position[1];
    const float z = pose.position[2];
    const bool isTrackingReference = deviceClass == ovtr::DeviceClass::TrackingReference;
    const float radius = isTrackingReference ? 0.0325f : 0.02f;
    constexpr float axisLength = 0.08f;

    if (selected) {
        glColor3f(1.0f, 0.12f, 0.10f);
    } else if ((pose.flags & ovtr::PoseFlagPoseValid) == 0) {
        glColor3f(0.55f, 0.55f, 0.55f);
    } else if (isTrackingReference) {
        glColor3f(0.72f, 0.42f, 1.0f);
    } else if (pose.runtimeIndex == 0) {
        glColor3f(0.25f, 0.95f, 0.95f);
    } else {
        glColor3f(1.0f, 0.72f, 0.24f);
    }

    ScopedGlMatrixStack modelView(GL_MODELVIEW);
    glTranslatef(x, y, z);
    multiplyOpenGLMatrixFromQuaternion(pose.rotation);

    if (drawBody) {
        glBegin(GL_QUADS);
        glVertex3f(-radius, -radius, radius);
        glVertex3f(radius, -radius, radius);
        glVertex3f(radius, radius, radius);
        glVertex3f(-radius, radius, radius);

        glVertex3f(-radius, -radius, -radius);
        glVertex3f(-radius, radius, -radius);
        glVertex3f(radius, radius, -radius);
        glVertex3f(radius, -radius, -radius);

        glVertex3f(-radius, radius, -radius);
        glVertex3f(-radius, radius, radius);
        glVertex3f(radius, radius, radius);
        glVertex3f(radius, radius, -radius);

        glVertex3f(-radius, -radius, -radius);
        glVertex3f(radius, -radius, -radius);
        glVertex3f(radius, -radius, radius);
        glVertex3f(-radius, -radius, radius);

        glVertex3f(radius, -radius, -radius);
        glVertex3f(radius, radius, -radius);
        glVertex3f(radius, radius, radius);
        glVertex3f(radius, -radius, radius);

        glVertex3f(-radius, -radius, -radius);
        glVertex3f(-radius, -radius, radius);
        glVertex3f(-radius, radius, radius);
        glVertex3f(-radius, radius, -radius);
        glEnd();
    }

    ScopedGlLineWidth lineWidth(1.5f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.20f, 0.20f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(axisLength, 0.0f, 0.0f);
    glColor3f(0.20f, 1.0f, 0.35f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, axisLength, 0.0f);
    glColor3f(0.35f, 0.55f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, axisLength);
    glEnd();
}

} // namespace ovtr::win32
